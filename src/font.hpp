#ifndef __FONT_HPP__
#define __FONT_HPP__

#include <vector>

#include "renderer.hpp"
#include "stb_truetype.h"
#include "lt_math.hpp"
#include "vertex.hpp"

struct AsciiFontAtlas
{
    std::string fontpath;
    stbtt_packedchar char_data[256];
    i32 width = 0;
    i32 height = 0;
    u8 *bitmap = nullptr;
    u32 id;
    u32 vao, vbo;

    inline bool is_valid() { return bitmap != nullptr; }
    void load(f32 font_size, i32 width, i32 height);

    AsciiFontAtlas();
    AsciiFontAtlas(const std::string &fontpath);
    AsciiFontAtlas(AsciiFontAtlas &&atlas);
    ~AsciiFontAtlas();

    void render_text_to_buffer(const std::string &text, Vec3f color, f32 start_xpos, f32 start_ypos,
                               std::vector<Vertex_PUC> &buf);
};

#endif // __FONT_HPP__
