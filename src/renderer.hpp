#ifndef __RENDERER_HPP__
#define __RENDERER_HPP__

struct World;
struct Camera;
struct Shader;
struct Application;
struct Skybox;
struct Mesh;

void render_world(const World &world);
void render_final_quad(const Application &app, const Camera &camera, Shader *shader);
void render_skybox(const Skybox &skybox);

void render_loading_screen();

// Mesh layouts
void render_setup_mesh_buffers_p(Mesh *m);

#endif // __RENDERER_HPP__
