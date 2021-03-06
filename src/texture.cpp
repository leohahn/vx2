#include "texture.hpp"
#include "lt_utils.hpp"
#include "application.hpp"
#include "io_task_manager.hpp"
#include "resource_manager.hpp"
#include "stb_image_write.h"
#include "vertex_buffer.hpp"

lt_global_variable lt::Logger logger("texture");

Texture::Texture(TextureType type, TextureFormat tf, PixelFormat pf)
    : id(0)
    , type(type)
    , texture_format(tf)
    , pixel_format(pf)
    , m_is_loaded(false)
{
    glGenTextures(1, &id);
}

Texture::~Texture()
{
    glDeleteTextures(1, &id);
}

// -----------------------------------------------------------------------------
// Cubemap
// -----------------------------------------------------------------------------

TextureCubemap::TextureCubemap(TextureFormat tf, PixelFormat pf, std::string *paths, IOTaskManager *manager)
    : Texture(TextureType_Cubemap, tf, pf)
    , m_task(nullptr)
    , m_io_task_manager(manager)
{
    LT_Assert(id != 0);
    for (i32 i = 0; i < NUM_CUBEMAP_FACES; i++)
        filepaths[i] = paths[i];

    logger.log("Creating cubemap");
}

bool
TextureCubemap::load()
{
    if (!m_task && !m_is_loaded)
    {
        logger.log("Adding task to queue.");
        m_task = std::make_unique<LoadImagesTask>(filepaths, NUM_CUBEMAP_FACES);
        m_io_task_manager->add_to_queue(m_task.get());
    }

    if (m_task && m_task->status() == TaskStatus_Complete)
    {
        logger.log("Creating texture");
        {
            const std::vector<std::unique_ptr<LoadedImage>> &loaded_images = m_task->view_loaded_images();
            LT_Assert(loaded_images.size() == 6);

            glBindTexture(GL_TEXTURE_CUBE_MAP, id);

            for (u32 i = 0; i < loaded_images.size(); i++)
            {
                const auto &li = loaded_images[i];

                const i32 mipmap_level = 0;
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, mipmap_level,
                             texture_format, li->width, li->height,
                             0, pixel_format, GL_UNSIGNED_BYTE, li->data);
            }

            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

            dump_opengl_errors("After cubemap creation");
        }
        logger.log("id ", id);
        m_task.reset(); // destroy task object and free its resources.
        m_is_loaded = true;
    }

    return m_is_loaded;
}

// -----------------------------------------------------------------------------
// Texture Atlas
// -----------------------------------------------------------------------------

TextureAtlas::TextureAtlas(TextureFormat tf, PixelFormat pf, const std::string &filepath,
                           i32 num_layers, i32 layer_width, i32 layer_height, IOTaskManager *manager)
    : Texture(TextureType_2D_Array, tf, pf)
    , filepath(filepath)
    , num_layers(num_layers)
    , layer_width(layer_width)
    , layer_height(layer_height)
    , m_task(nullptr)
    , m_io_task_manager(manager)
{
    LT_Assert(id != 0);
    logger.log("Creating texture atlas ", filepath);
}

bool
TextureAtlas::load()
{
    if (!m_task && !m_is_loaded)
    {
        logger.log("Adding task to queue.");
        m_task = std::make_unique<LoadImagesTask>(&filepath, 1);
        m_io_task_manager->add_to_queue(m_task.get());
    }

    if (m_task && m_task->status() == TaskStatus_Complete)
    {
        logger.log("Creating texture");
        {
            const std::vector<std::unique_ptr<LoadedImage>> &loaded_images = m_task->view_loaded_images();
            LT_Assert(loaded_images.size() == 1);

            width = loaded_images[0]->width;
            height = loaded_images[0]->height;

            logger.log("Texture sized (", width, ", ", height, ")");
            logger.log("Num layers: ", num_layers);

            LT_Assert(width == layer_width);
            LT_Assert(height == num_layers*layer_height);

            glBindTexture(GL_TEXTURE_2D_ARRAY, id);
            const i32 mipmap_level = 0;

            glTexImage3D(GL_TEXTURE_2D_ARRAY, mipmap_level, texture_format,
                         layer_width, layer_height, num_layers,
                         0, pixel_format, GL_UNSIGNED_BYTE, loaded_images[0]->data);

            // TODO: Should these be parameters?
            glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);

            dump_opengl_errors("After texture 2D array creation");
        }
        logger.log(filepath, ": id ", id);
        m_task.reset(); // destroy task object and free its resources.
        m_is_loaded = true;
    }

    return m_is_loaded;
}

// -----------------------------------------------------------------------------
// Shadow Map
// -----------------------------------------------------------------------------
lt_internal Mesh
create_debug_render_mesh(const char *texture_name, u32 texture_id)
{
    Mesh mesh = {};

    const isize NUM_VERTICES = LT_Count(UNIT_PLANE_VERTICES);
    const isize NUM_INDICES = LT_Count(UNIT_PLANE_INDICES);

    mesh.vertices = std::vector<Vec3f>(NUM_VERTICES);
    mesh.tex_coords = std::vector<Vec2f>(NUM_VERTICES);

    // Add all only the positions
    for (usize i = 0; i < NUM_VERTICES; i++)
    {
        mesh.vertices[i] = UNIT_PLANE_VERTICES[i];
        mesh.tex_coords[i] = UNIT_PLANE_TEX_COORDS[i];
    }

    for (usize i = 0; i < NUM_INDICES; i+=3)
    {
        Face face = {};
        face.val[0] = UNIT_PLANE_INDICES[i];
        face.val[1] = UNIT_PLANE_INDICES[i+1];
        face.val[2] = UNIT_PLANE_INDICES[i+2];
        mesh.faces.push_back(face);
    }

    Submesh sm = {};
    sm.start_index = 0;
    sm.num_indices = mesh.num_indices();
    sm.textures.push_back(TextureInfo(texture_id, texture_name, GL_TEXTURE_2D));
    mesh.submeshes.push_back(sm);

    VertexBuffer::setup_pu(mesh);

    return mesh;
}

ShadowMap::ShadowMap(i32 width, i32 height, const char *shader_name,
                     const char *debug_shader_name, const ResourceManager &manager)
    : shader(manager.get_shader(shader_name))
    , width(width)
    , height(height)
    , debug_render_shader(manager.get_shader(debug_shader_name))
{
    LT_Assert(shader);

    // Create the texture
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    const Vec4f border_color(1.0f, 1.0f, 1.0f, 1.0f);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, &border_color.val[0]);

    // Attach texture to the framebuffer
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        LT_Panic("framebuffer not complete");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    LT_Assert(texture > 0);
    debug_render_quad = create_debug_render_mesh("texture_shadow_map", texture);
}

ShadowMap::ShadowMap(ShadowMap&& sm)
    : shader(sm.shader)
    , fbo(sm.fbo)
    , texture(sm.texture)
    , width(sm.width)
    , height(sm.height)
{
    sm.fbo = 0;
    sm.texture = 0;
    sm.shader = nullptr;
    sm.width = -1;
    sm.height = -1;
}

ShadowMap::~ShadowMap()
{
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &texture);
}

void
ShadowMap::bind_framebuffer() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}
