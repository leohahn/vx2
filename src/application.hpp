#ifndef __APPLICATION_HPP__
#define __APPLICATION_HPP__

#include "lt_core.hpp"
#include "mesh.hpp"

struct GLFWwindow;
struct Resources;

struct Application
{
    Application(const char *title, i32 width, i32 height);
    ~Application();

    void bind_default_framebuffer() const;

    inline f32 aspect_ratio() const
    {
        return (f32)screen_width/(f32)screen_height;
    }

public:
    GLFWwindow *window;
    const char *title;
    i32         screen_width;
    i32         screen_height;
    Mesh        render_quad;
};

void dump_opengl_errors(const char *func, const char *file = nullptr);


#endif // __APPLICATION_HPP__
