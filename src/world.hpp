#ifndef __WORLD_HPP__
#define __WORLD_HPP__

#include <memory>

#include "lt_core.hpp"
#include "lt_math.hpp"
#include "skybox.hpp"
#include "camera.hpp"
#include "landscape.hpp"

struct Key;
struct osn_context;
struct TextureAtlas;
struct Application;
struct Input;
struct TextureAtlas;

struct Sun
{
    Vec3f direction;
    Vec3f ambient;
    Vec3f diffuse;
    Vec3f specular;
};

enum WorldStatus
{
    WorldStatus_InitialLoad,
    WorldStatus_Running,
    WorldStatus_Paused,
};

struct WorldState
{
    Frustum camera_frustum;
};

enum Textures16x16 : u16
{
    Earth_Sides = 0,
    Earth_Sides_Top = 1,
    Earth_Top = 2,
    Earth_Bottom = 3,

    Snow_Sides = 4,
    Snow_Sides_Top = 5,
    Snow_Top = 6,
    Snow_Bottom = 7,

    Crosshair = 8,
};

struct World
{
    World(Application &app, i32 seed, const char *blocks_texture,
          const ResourceManager &manager, f32 aspect_ratio);

    static World interpolate(const World &pevious, const World &current, f32 alpha);

    void update(Input &input);
    void generate_landscape(f64 amplitude, f64 frequency, i32 num_octaves, f64 lacunarity, f64 gain);

    void update_chunk_buffer(i32 cx, i32 cy, i32 cz);

public:
    using LandscapePtr = std::shared_ptr<Landscape>;

    LandscapePtr           landscape;
    Camera                 camera;
    // NOTE: Different world instances only differ in values that are interpolated
    // based on the framerate. The chunks are not one of these.
    Skybox                 skybox;
    Sun                    sun;
    Vec3f                  origin;
    WorldStatus            state = WorldStatus_InitialLoad;
    bool                   render_wireframe = false;
    TextureAtlas          *textures_16x16;

private:
    Camera create_camera(Vec3f position, f32 aspect_ratio);
};

#endif // __WORLD_HPP__
