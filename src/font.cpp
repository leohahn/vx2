#include "font.hpp"
#include "lt_core.hpp"
#include "lt_utils.hpp"
#include "lt_fs.hpp"
#include "stb_truetype.h"
#include "stb_rect_pack.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "glad/glad.h"

lt_global_variable lt::Logger logger("font");

AsciiFontAtlas::AsciiFontAtlas()
    : width(0)
    , height(0)
    , bitmap(nullptr)
{}

AsciiFontAtlas::AsciiFontAtlas(const std::string &fontpath)
    : fontpath(fontpath)
    , width(0)
    , height(0)
    , bitmap(nullptr)
    , id(0)
{
}

AsciiFontAtlas::~AsciiFontAtlas()
{
    delete[] bitmap;
    glDeleteTextures(1, &id);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
}

AsciiFontAtlas::AsciiFontAtlas(AsciiFontAtlas &&atlas)
{
    logger.log("Calling move contructor");
    width = atlas.width;
    height = atlas.height;
    bitmap = atlas.bitmap;
    atlas.bitmap = nullptr;
}

void
AsciiFontAtlas::load(f32 font_size, i32 _width, i32 _height)
{
    width = _width;
    height = _height;

    bitmap = new u8[width*height];

    FileContents *ttf_buffer = file_read_contents(fontpath.c_str());
    if (ttf_buffer->error != FileError_None)
    {
        logger.error("Failed to open font file in ", fontpath);
    }

    stbtt_pack_context context;

    const i32 stride_in_bytes = 0;
    const i32 padding = 1;
    if (stbtt_PackBegin(&context, bitmap, width, height,
                        stride_in_bytes, padding, nullptr))
    {
        stbtt_PackSetOversampling(&context, 2, 2);
        stbtt_PackFontRange(&context, (u8*)ttf_buffer->data, 0, font_size, 0, 256, char_data);
        // LT_Assert(stbi_write_png("bitmap.png", width, height, 1, (void*)bitmap, width));
        stbtt_PackEnd(&context);

        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
    }
    else
    {
        logger.error("Failed to create a packing context.");
    }

    file_free_contents(ttf_buffer);
}

void
AsciiFontAtlas::render_text_to_buffer(const std::string &text, Vec3f color, f32 start_xpos, f32 start_ypos,
                                      std::vector<Vertex_PUC> &buf)
{
    float xpos = start_xpos;
    float ypos = start_ypos;
    for (u32 i = 0; i < text.size(); i++)
    {
        if (text[i] == '\n')
        {
            xpos = start_xpos;
            ypos += 20.0f;
            continue;
        }

        stbtt_aligned_quad quad;
        stbtt_GetPackedQuad(char_data, width, height, text[i], &xpos, &ypos, &quad, true);

        Vertex_PUC top_left(Vec3f(quad.x0, quad.y0, 0), Vec2f(quad.s0, quad.t0), color);
        Vertex_PUC bottom_right(Vec3f(quad.x1, quad.y1, 0), Vec2f(quad.s1, quad.t1), color);
        Vertex_PUC top_right(Vec3f(quad.x1, quad.y0, 0), Vec2f(quad.s1, quad.t0), color);
        Vertex_PUC bottom_left(Vec3f(quad.x0, quad.y1, 0), Vec2f(quad.s0, quad.t1), color);

        buf.push_back(top_left);
        buf.push_back(bottom_left);
        buf.push_back(bottom_right);
        buf.push_back(bottom_right);
        buf.push_back(top_right);
        buf.push_back(top_left);
    }
}
