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
struct ShadowMap;
struct TextureAtlas;

struct UiState
{
    enum Selection : u32
    {
        Selection_Resume = 0,
        Selection_Quit,
        Selection_Count,
    };

    Selection current_selection = Selection_Resume;

    void next_selection()
    {
        current_selection = static_cast<Selection>((current_selection+1) % Selection_Count);
    }
    void prev_selection()
    {
        if (current_selection == 0)
            current_selection = static_cast<Selection>(Selection_Count-1);
        else
            current_selection = static_cast<Selection>(current_selection-1);
    }
};

struct Sun
{
    Vec3f direction;
    Vec3f ambient;
    Vec3f diffuse;
    Vec3f specular;
    // NOTE: Position is only considered for calculating the orthographic matrix in shadow mapping.
    Vec3f position;
    Mat4f projection;

    void update_position(Vec3f landscape_origin);

    inline Mat4f light_space() const
    {
        return projection * lt::look_at(position, position+direction, Vec3f(0, 1, 0));
    }
};

enum WorldStatus
{
    WorldStatus_InitialLoad,
    WorldStatus_Running,
    WorldStatus_Paused,
    WorldStatus_Finished,
};

struct WorldState
{
    Frustum camera_frustum;
};

enum Textures16x16 : u16
{
    Textures16x16_Earth_Sides = 0,
    Textures16x16_Earth_Sides_Top = 1,
    Textures16x16_Earth_Top = 2,
    Textures16x16_Earth_Bottom = 3,

    Textures16x16_Snow_Sides = 4,
    Textures16x16_Snow_Sides_Top = 5,
    Textures16x16_Snow_Top = 6,
    Textures16x16_Snow_Bottom = 7,

    Textures16x16_Crosshair = 8,
};

struct Crosshair
{
    Mesh quad;
    Shader *shader;

    Crosshair(const char *shader_name, const ResourceManager &manager, u32 texture_id);
};

struct World
{
    World(Application &app, i32 seed, const char *blocks_texture,
          const ResourceManager &manager, f32 aspect_ratio);

    static World interpolate(const World &pevious, const World &current, f32 alpha);

    void update(Input &input, ShadowMap &shadow_map, Shader *basic_shader);
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
    WorldStatus            status = WorldStatus_InitialLoad;
    TextureAtlas          *textures_16x16;
    Crosshair              crosshair;
    Vec3f                  sky_color;
    UiState                ui_state;

private:
    Camera create_camera(Vec3f position, f32 aspect_ratio);
    void update_status(const Input &input);
};

#endif // __WORLD_HPP__
