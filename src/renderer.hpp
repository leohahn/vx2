#ifndef __RENDERER_HPP__
#define __RENDERER_HPP__

struct World;
struct Camera;
struct Shader;
struct Application;

void render_world(const World &world, const Camera &camera, Shader *shader);
void render_final_quad(const Application &app, const Camera &camera, Shader *shader);

#endif // __RENDERER_HPP__
