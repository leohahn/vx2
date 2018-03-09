#ifndef __APPLICATION_HPP__
#define __APPLICATION_HPP__

#include "lt_core.hpp"
#include "mesh.hpp"
#include "landscape.hpp"
#include "input.hpp"

struct GLFWwindow;
struct Resources;

struct Memory
{
    Memory()
        // This memory is used with a pool allocator.
        : chunks_memory_size(sizeof(Landscape::Chunk) * 1575)
        , chunks_memory(calloc(1, chunks_memory_size))
    {}

    ~Memory()
    {
        free(chunks_memory);
    }

public:
    usize chunks_memory_size;
    void *chunks_memory;
};

struct Application
{
    Application(const char *title, i32 width, i32 height);
    ~Application();

    void bind_default_framebuffer() const;
    void process_input();

    inline f32 aspect_ratio() const
    {
        return (f32)screen_width/(f32)screen_height;
    }

    bool should_close() const;

public:
    GLFWwindow *window;
    const char *title;
    i32         screen_width;
    i32         screen_height;
    Mesh        render_quad;
    Memory      memory;
    Input       input;

private:
    void reset_mouse_position();
};

void dump_opengl_errors(const char *func, const char *file = nullptr);


#endif // __APPLICATION_HPP__
