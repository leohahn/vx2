/* ====================================
 *
 *   Vertex Shader
 *
 * ==================================== */
#ifdef COMPILING_VERTEX

layout (location = 0) in vec3 att_position;
layout (location = 1) in vec3 att_tex_coords_layer;
layout (location = 2) in vec3 att_normal;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 light_space;

out VS_OUT
{
    vec3 frag_world_pos;
    vec3 frag_tex_coords_layer;
    vec3 frag_normal;
    vec4 frag_pos_light_space;
    float visibility;
} vs_out;

void
main()
{
    vs_out.frag_world_pos = att_position;
    vs_out.frag_tex_coords_layer = att_tex_coords_layer;
    vs_out.frag_normal = att_normal;
    vs_out.frag_pos_light_space = light_space * vec4(vs_out.frag_world_pos, 1.0f);

    vec4 pos_in_camera_space = view * vec4(att_position, 1.0);

    const float density = 0.008;
    const float gradient = 2.0;

    float distance_from_camera = length(pos_in_camera_space.xyz);
    vs_out.visibility = exp(-pow(distance_from_camera*density, gradient));
    vs_out.visibility = clamp(vs_out.visibility, 0.0, 1.0);

    gl_Position = projection * pos_in_camera_space;
}

#endif

/* ====================================
 *
 *   Fragment Shader
 *
 * ==================================== */
#ifdef COMPILING_FRAGMENT

in VS_OUT
{
    vec3 frag_world_pos;
    vec3 frag_tex_coords_layer;
    vec3 frag_normal;
    vec4 frag_pos_light_space;
    float visibility;
} vs_out;

out vec4 frag_color;

struct Sun
{
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform sampler2DArray texture_array;
uniform sampler2D texture_shadow_map;
uniform samplerCube texture_cubemap;

uniform vec3 view_position;
uniform Sun sun;
uniform vec3 sky_color;

#define NUM_LAYERS 9

vec3
apply_gamma_correction(vec3 color)
{
    const float gamma = 2.2;
    return pow(color, vec3(1.0/gamma));
}

float
shadow_calculation(vec4 pos_light_space)
{
    const int mipmap_lvl = 0;
    const float texel_offset = 1.0;
    const int window_side = 3;

    int num_sampled_texels = window_side*window_side;
    int offset_xy = window_side/2;

    // perspective divide and map coordinates to the texture's one
    vec3 projection_coords = pos_light_space.xyz / pos_light_space.w;
    projection_coords = projection_coords * 0.5 + 0.5;

    // Return no shadow if the fragment is outside the far plane of the light space
    if (projection_coords.z > 1.0)
        return 0.0;

    // Implement Percentage-Closer Filtering
    float frag_depth = projection_coords.z;
    float shadow = 0.0;
    vec2 texel_size = texel_offset / textureSize(texture_shadow_map, mipmap_lvl);

    // Get depth values for a 3x3 neighborhood, then average by 9 (number of neighbors)
    for (int y = -offset_xy; y <= offset_xy; y++)
        for (int x = -offset_xy; x <= offset_xy; x++)
        {
            float depth = texture2D(texture_shadow_map, projection_coords.xy + vec2(x, y)*texel_size).r;
            shadow += float(frag_depth > depth);
            // shadow += depth;
        }
    shadow /= num_sampled_texels;
    return shadow;
}

vec3
calc_directional_light(Sun sun, vec3 frag_albedo, float frag_specular, vec3 frag_normal,
                       vec4 frag_pos_light_space)
{
    const float shininess = 128;

    vec3 frag_to_light = -sun.direction;
    vec3 frag_to_view = normalize(view_position - vs_out.frag_world_pos);
    vec3 halfway_dir = normalize(frag_to_light + frag_to_view);

    vec3 ambient_component = sun.ambient * frag_albedo * vec3(0.04f);

    float diffuse_strength = max(0.0f, dot(frag_to_light, frag_normal));
    vec3 diffuse_component = sun.diffuse * diffuse_strength * frag_albedo;

    // float specular_strength = pow(max(0.0f, dot(halfway_dir, frag_normal)), shininess);
    // vec3 specular_component = sun.specular * (specular_strength * frag_specular);

    float shadow = shadow_calculation(frag_pos_light_space);
    return (ambient_component + diffuse_component*(1-shadow));

    // return (ambient_component + diffuse_component);
}

void
main()
{
    float layer = max(0, min(NUM_LAYERS-1, floor(vs_out.frag_tex_coords_layer.z + 0.5)));

    vec3 albedo = texture(texture_array, vec3(vs_out.frag_tex_coords_layer.xy, layer)).rgb;
    vec3 sun_contribution = calc_directional_light(sun, albedo, 0.3, vs_out.frag_normal,
                                                   vs_out.frag_pos_light_space);

    vec3 color = sun_contribution;
    color = apply_gamma_correction(color);

    frag_color = vec4(color, 1.0);
    frag_color = mix(vec4(sky_color, 1.0), frag_color, vs_out.visibility);
}

#endif
