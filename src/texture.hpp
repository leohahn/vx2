#ifndef __TEXTURE_HPP__
#define __TEXTURE_HPP__

#include <vector>
#include <string>
#include <memory>
#include <array>

#include "glad/glad.h"
#include "lt_core.hpp"
#include "io_task.hpp"

struct IOTaskManager;
// struct LoadImagesTask;

enum TextureFormat
{
    TextureFormat_RGB = GL_RGB8,
    TextureFormat_RGBA = GL_RGBA,
    TextureFormat_SRGB = GL_SRGB8,
    TextureFormat_SRGBA = GL_SRGB_ALPHA,
};

enum PixelFormat
{
    PixelFormat_RGB = GL_RGB,
    PixelFormat_RGBA = GL_RGBA,
};

enum TextureType
{
    TextureType_Unknown,
    TextureType_2D,
    TextureType_2D_Array,
    TextureType_Cubemap,
};

struct Texture
{
    Texture(TextureType type, TextureFormat tf, PixelFormat pf);
    virtual ~Texture();

    virtual bool load() = 0;
    inline bool is_loaded() const { return m_is_loaded; }

    u32 id;
    TextureType type;
    TextureFormat texture_format;
    PixelFormat pixel_format;

protected:
    bool m_is_loaded;
};

constexpr i32 NUM_CUBEMAP_FACES = 6;

struct TextureCubemap : Texture
{
    TextureCubemap(TextureFormat tf, PixelFormat pf, std::string *paths, IOTaskManager *manager);
    ~TextureCubemap() {}

    bool load() override;

public:
    std::string filepaths[NUM_CUBEMAP_FACES];

private:
    std::unique_ptr<LoadImagesTask> m_task;
    IOTaskManager *m_io_task_manager;
};

struct TextureAtlas : Texture
{
    TextureAtlas(TextureFormat tf, PixelFormat pf, const std::string &filepath,
                 i32 num_layers, i32 layer_width, i32 layer_height, IOTaskManager *manager);
    ~TextureAtlas() {}

    bool load() override;

public:
    std::string filepath;
    i32 width;
    i32 height;
    i32 num_layers;
    i32 layer_width;
    i32 layer_height;

private:
    std::unique_ptr<LoadImagesTask> m_task;
    IOTaskManager *m_io_task_manager;
};

#endif // __TEXTURE_HPP__
