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

struct Vertex_PU
{
    Vec3f position;
    Vec2f tex_coords;

    Vertex_PU(Vec3f p, Vec2f t) : position(p), tex_coords(t) {}
    Vertex_PU() : position(Vec3f(0)), tex_coords(Vec2f(0)) {}
};

void render_world(const World &world);
void render_final_quad(const Application &app, const Camera &camera, Shader *shader);
void render_skybox(const Skybox &skybox);
void render_text(const std::string &text, Shader &shader, const std::vector<Vertex_PU> &vertexes);
void render_loading_screen();

// Mesh layouts
void render_setup_mesh_buffers_p(Mesh *m);

#endif // __RENDERER_HPP__
