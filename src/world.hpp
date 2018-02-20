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

struct World
{
    static World interpolate(const World &pevious, const World &current, f32 alpha);

    World(i32 seed, const char *blocks_texture, const ResourceManager &manager, f32 aspect_ratio);

    void update(Key *kb);
    void generate_landscape(f64 amplitude, f64 frequency, i32 num_octaves, f64 lacunarity, f64 gain);

    void update_chunk_buffer(i32 cx, i32 cy, i32 cz);

public:
    using LandscapePtr = std::shared_ptr<Landscape>;

    LandscapePtr           landscape;
    Camera                 camera;
    // NOTE: Different world instances only differ in values that are interpolated
    // based on the framerate. The chunks are not one of these.
    Skybox                 skybox;
    BlocksTextureInfo      blocks_texture_info;
    Sun                    sun;
    Vec3f                  origin;
    WorldStatus            state = WorldStatus_InitialLoad;
    bool                   render_wireframe = false;

private:
    Camera create_camera(f32 aspect_ratio);
};

#endif // __WORLD_HPP__
