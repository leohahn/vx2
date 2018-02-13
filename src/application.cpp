#include "application.hpp"
#include <cstring>
#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include "resources.hpp"
#include "lt_utils.hpp"
#include "unit_plane_data.hpp"

lt_global_variable lt::Logger logger("application");
lt_global_variable lt::Logger gl_logger("OpenGL");

lt_internal void
framebuffer_size_callback(GLFWwindow *w, i32 width, i32 height)
{
    LT_Unused(w);
    glViewport(0, 0, width, height);
}

void
dump_opengl_errors(const char *func, const char *file)
{
#ifdef LT_DEBUG
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
    {
        lt_local_persist char desc[50] = {};
        switch (err)
        {
        case GL_INVALID_ENUM: {
            const char *s = "Invalid enumeration.";
            std::sprintf(desc, "%s", s);
            break;
        }
        case GL_INVALID_VALUE: {
            const char *s = "Invalid value.";
            std::sprintf(desc, "%s", s);
            break;
        }
        case GL_INVALID_OPERATION: {
            const char *s = "Invalid operation.";
            std::sprintf(desc, "%s", s);
            break;
        }
        case GL_INVALID_FRAMEBUFFER_OPERATION: {
            const char *s = "Invalid framebuffer operation.";
            std::sprintf(desc, "%s", s);
            break;
        }
        case GL_OUT_OF_MEMORY: {
            const char *s = "Out of memory";
            std::sprintf(desc, "%s", s);
            break;
        }
        default: LT_Assert(false);
        }
        if (file)
            gl_logger.log(func, " (", file, ") : ", desc);
        else
            gl_logger.log(func,": ", desc);
    }
#endif
}

lt_internal void
setup_mesh_buffers_pu(Mesh &m)
{
    // Temporary vertex buffer to be deallocated at the end of the function.
    std::vector<Vertex_PU> vertexes_buf(m.vertices.size());
    for (usize i = 0; i < m.vertices.size(); i++)
    {
        vertexes_buf[i].position = m.vertices[i];
        vertexes_buf[i].tex_coords = m.tex_coords[i];
    }

    glGenVertexArrays(1, &m.vao);
    glGenBuffers(1, &m.vbo);
    glGenBuffers(1, &m.ebo);

    glBindVertexArray(m.vao);

    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertexes_buf.size() * sizeof(Vertex_PU), &vertexes_buf[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m.faces.size() * sizeof(Face), &m.faces[0], GL_STATIC_DRAW);

    // vertex positions
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_PU), (void*)offsetof(Vertex_PU, position));
    glEnableVertexAttribArray(0);
    // vertex texture coords
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex_PU), (void*)offsetof(Vertex_PU, tex_coords));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

Mesh
Application::create_render_quad()
{
    const i32 NUM_VERTICES = LT_Count(UNIT_PLANE_VERTICES);
    const i32 NUM_INDICES = LT_Count(UNIT_PLANE_INDICES);

    Mesh mesh = {};
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
    sm.textures.push_back(TextureInfo(gbuffer.position_buffer, "texture_position", GL_TEXTURE_2D));
    sm.textures.push_back(TextureInfo(gbuffer.normal_buffer, "texture_normal", GL_TEXTURE_2D));
    sm.textures.push_back(TextureInfo(gbuffer.albedo_specular_buffer, "texture_albedo_specular", GL_TEXTURE_2D));
    mesh.submeshes.push_back(sm);

    setup_mesh_buffers_pu(mesh);
    return mesh;
}

Application::~Application()
{
    logger.log("Releasing application resources.");
    // free gbuffer resources
    glDeleteFramebuffers(1, &gbuffer.fbo);
    glDeleteTextures(1, &gbuffer.position_buffer);
    glDeleteTextures(1, &gbuffer.normal_buffer);
    glDeleteTextures(1, &gbuffer.albedo_specular_buffer);
    glDeleteRenderbuffers(1, &gbuffer.depth_stencil_buffer);
    // free mesh resources
    glDeleteVertexArrays(1, &render_quad.vao);
    glDeleteBuffers(1, &render_quad.vbo);
    glDeleteBuffers(1, &render_quad.ebo);
    // free GLFW resources
    glfwDestroyWindow(window);
    glfwTerminate();
}

void Application::bind_gbuffer() const {glBindFramebuffer(GL_FRAMEBUFFER, gbuffer.fbo);}
void Application::bind_default_framebuffer() const {glBindFramebuffer(GL_FRAMEBUFFER, 0);}

lt_internal void
setup_gbuffer(GBuffer &gbuffer, i32 width, i32 height)
{
    glGenFramebuffers(1, &gbuffer.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, gbuffer.fbo);

    u32 *buffers[] = {&gbuffer.position_buffer, &gbuffer.normal_buffer};
    u32 i;
    for (i = 0; i < LT_Count(buffers); i++)
    {
        // Position buffer
        glGenTextures(1, buffers[i]);
        glBindTexture(GL_TEXTURE_2D, *buffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
        dump_opengl_errors("glTexImage2D");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, *buffers[i], 0);
        dump_opengl_errors("glFramebufferTexture2D");
    }

    // albedo buffer + specular
    glGenTextures(1, &gbuffer.albedo_specular_buffer);
    glBindTexture(GL_TEXTURE_2D, gbuffer.albedo_specular_buffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    dump_opengl_errors("glTexImage2D");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D,
                           gbuffer.albedo_specular_buffer, 0);
    dump_opengl_errors("glFramebufferTexture2D");

    // Add depth buffer and stencil buffer
    glGenRenderbuffers(1, &gbuffer.depth_stencil_buffer);
    glBindRenderbuffer(GL_RENDERBUFFER, gbuffer.depth_stencil_buffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    dump_opengl_errors("glRenderbufferStorage");
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER, gbuffer.depth_stencil_buffer);
    dump_opengl_errors("glFramebufferRenderbuffer");

    const u32 attachments[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
    glDrawBuffers(3, attachments);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        logger.error("GBuffer is incomplete.");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

Application::Application(const char *title, i32 width, i32 height)
    : title(title)
    , screen_width(width)
    , screen_height(height)
{
    logger.log("Creating the application.");

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, false);
    glfwWindowHint(GLFW_SAMPLES, 4);

    window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        LT_Fail("Failed to create glfw window.\n");
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        LT_Fail("Failed to initialize GLAD\n");

    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    glViewport(0, 0, width, height);

    logger.log("Setting up the gbuffer");
    setup_gbuffer(gbuffer, width, height);

    logger.log("Creating render quad");
    render_quad = create_render_quad();
}
