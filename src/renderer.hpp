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

void render_world(World &world);
void render_final_quad(const Application &app, const Camera &camera, Shader *shader);
void render_skybox(const Skybox &skybox);
void render_text(AsciiFontAtlas *atlas, const std::string &text, f32 posx, f32 posy, Shader *shader);
void render_loading_screen(const Application &app, AsciiFontAtlas *atlas, Shader *font_shader);

// Mesh layouts
void render_setup_mesh_buffers_p(Mesh *m);

#endif // __RENDERER_HPP__
