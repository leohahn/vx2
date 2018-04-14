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
struct Vertex_PUC;
struct ResourceManager;
struct Frustum;

void render_landscape(World &world);
void render_skybox(const Skybox &skybox);
void render_text(AsciiFontAtlas *atlas, const std::string &text, f32 posx, f32 posy, Shader *shader);
void render_loading_screen(const Application &app, AsciiFontAtlas *atlas, Shader *font_shader);
void render_mesh(const Mesh &mesh, Shader *shader);

// Mesh layouts
void render_setup_mesh_buffers_p(Mesh *m);

// Debug rendering
void debug_render_frustum(const Frustum &frustum);

// UI
struct UiRenderer
{
    std::vector<Vertex_PUC> text_vertexes;
    u32 text_vao, text_vbo;
    Shader *font_shader;
    AsciiFontAtlas *font_atlas;

    UiRenderer(const char *font_shader_name, const char *font_name, ResourceManager &manager);
    ~UiRenderer();

    void begin();
    void flush();
    void text(const std::string &text, Vec3f color, f32 xpos, f32 ypos);

public:
    UiRenderer(const UiRenderer &) = delete;
    UiRenderer(const UiRenderer &&) = delete;
    UiRenderer &operator=(const UiRenderer &) = delete;
    UiRenderer &operator=(const UiRenderer &&) = delete;
};

#endif // __RENDERER_HPP__
