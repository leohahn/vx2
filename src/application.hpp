#ifndef __APPLICATION_HPP__
#define __APPLICATION_HPP__

#include "lt_core.hpp"
#include "mesh.hpp"

struct GLFWwindow;
struct Resources;

struct GBuffer
{
    u32 fbo;
    u32 position_buffer;
    u32 normal_buffer;
    u32 albedo_specular_buffer;
    u32 depth_stencil_buffer;
};

struct Application
{
    GLFWwindow *window;
    const char *title;
    i32         screen_width;
    i32         screen_height;
    GBuffer     gbuffer;
    Mesh        render_quad;

    Application(const char *title, i32 width, i32 height);

    inline f32 aspect_ratio() const
    {
        return (f32)screen_width/(f32)screen_height;
    }

    void bind_gbuffer() const;
    void bind_default_framebuffer() const;

    ~Application();

private:
    Mesh create_render_quad();
};

void dump_opengl_errors(const char *func, const char *file = nullptr);


#endif // __APPLICATION_HPP__
