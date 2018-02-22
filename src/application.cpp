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

// lt_internal void
// setup_mesh_buffers_pu(Mesh &m)
// {
//     // Temporary vertex buffer to be deallocated at the end of the function.
//     std::vector<Vertex_PU> vertexes_buf(m.vertices.size());
//     for (usize i = 0; i < m.vertices.size(); i++)
//     {
//         vertexes_buf[i].position = m.vertices[i];
//         vertexes_buf[i].tex_coords = m.tex_coords[i];
//     }

//     glGenVertexArrays(1, &m.vao);
//     glGenBuffers(1, &m.vbo);
//     glGenBuffers(1, &m.ebo);

//     glBindVertexArray(m.vao);

//     glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
//     glBufferData(GL_ARRAY_BUFFER, vertexes_buf.size() * sizeof(Vertex_PU), &vertexes_buf[0], GL_STATIC_DRAW);

//     glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ebo);
//     glBufferData(GL_ELEMENT_ARRAY_BUFFER, m.faces.size() * sizeof(Face), &m.faces[0], GL_STATIC_DRAW);

//     // vertex positions
//     glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_PU), (void*)offsetof(Vertex_PU, position));
//     glEnableVertexAttribArray(0);
//     // vertex texture coords
//     glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex_PU), (void*)offsetof(Vertex_PU, tex_coords));
//     glEnableVertexAttribArray(1);

//     glBindVertexArray(0);
// }

Application::~Application()
{
    logger.log("Releasing application resources.");
    // free mesh resources
    glDeleteVertexArrays(1, &render_quad.vao);
    glDeleteBuffers(1, &render_quad.vbo);
    glDeleteBuffers(1, &render_quad.ebo);
    // free GLFW resources
    glfwDestroyWindow(window);
    glfwTerminate();
}

void Application::bind_default_framebuffer() const {glBindFramebuffer(GL_FRAMEBUFFER, 0);}

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
    // This does not work however for me.
    glfwSwapInterval(0); // Set vsync off

    window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        LT_Panic("Failed to create glfw window.\n");
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        LT_Panic("Failed to initialize GLAD\n");

    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    glViewport(0, 0, width, height);
}
