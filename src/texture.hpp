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
    TextureType_Cubemap,
};

struct Texture
{
    Texture(TextureType type, TextureFormat tf, PixelFormat pf)
        : id(0), type(type), texture_format(tf), pixel_format(pf)
    {}
    virtual ~Texture() {}

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
                 i32 num_tile_rows, i32 num_tile_cols, i32 tile_width, i32 tile_height,
                 IOTaskManager *manager);
    ~TextureAtlas() {}

    bool load() override;

public:
    std::string filepath;

private:
    std::unique_ptr<LoadImagesTask> m_task;
    IOTaskManager *m_io_task_manager;

    i32 m_num_tile_rows;
    i32 m_num_tile_cols;
    i32 m_tile_width;
    i32 m_tile_height;
};

#endif // __TEXTURE_HPP__
