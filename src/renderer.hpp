#ifndef __RENDERER_HPP__
#define __RENDERER_HPP__

#include "lt_math.hpp"
#include <vector>

struct World;
struct Camera;
struct Shader;
struct Application;
struct Skybox;
struct Mesh;
struct AsciiFontAtlas;
struct Vertex_PU;
struct ResourceManager;

void render_landscape(World &world);
// void render_final_quad(const Application &app, const Camera &camera, Shader *shader);
void render_skybox(const Skybox &skybox);
void render_text(AsciiFontAtlas *atlas, const std::string &text, f32 posx, f32 posy, Shader *shader);
void render_loading_screen(const Application &app, AsciiFontAtlas *atlas, Shader *font_shader);
void render_mesh(const Mesh &mesh, Shader *shader);

// Mesh layouts
void render_setup_mesh_buffers_p(Mesh *m);

// UI
struct UiRenderer
{
    std::vector<Vertex_PU> text_vertexes;
    u32 text_vao, text_vbo;
    Shader *font_shader;
    AsciiFontAtlas *font_atlas;

    UiRenderer(const char *font_shader_name, ResourceManager &manager);
    ~UiRenderer();

    void begin();
    void flush();

public:
    UiRenderer(const UiRenderer &) = delete;
    UiRenderer(const UiRenderer &&) = delete;
    UiRenderer &operator=(const UiRenderer &) = delete;
    UiRenderer &operator=(const UiRenderer &&) = delete;
};

#endif // __RENDERER_HPP__
