/* ====================================
 *
 *   Vertex Shader
 *
 * ==================================== */
#ifdef COMPILING_VERTEX

layout (location = 0) in vec3 att_position;
layout (location = 1) in vec2 att_tex_coords;

out VS_OUT
{
    vec2 frag_tex_coords;
    vec3 frag_world_pos;
} vs_out;

void
main()
{
    vs_out.frag_tex_coords = att_tex_coords;
    vs_out.frag_world_pos = att_position;
    gl_Position = vec4(att_position, 1.0f);
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
    vec2 frag_tex_coords;
    vec3 frag_world_pos;
} vs_out;

struct Sun
{
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

out vec4 frag_color;

// gbuffer textures
uniform sampler2D texture_position;
uniform sampler2D texture_normal;
uniform sampler2D texture_albedo_specular;

uniform Sun sun;
uniform vec3 view_position;

vec3
apply_gamma_correction(vec3 color)
{
    const float gamma = 2.2;
    return pow(color, vec3(1.0/gamma));
}

vec3
calc_directional_light(Sun sun, vec3 frag_albedo, float frag_specular, vec3 frag_normal)
{
    const float shininess = 32;

    vec3 frag_to_light = -sun.direction;
    vec3 frag_to_view = normalize(view_position - vs_out.frag_world_pos);
    vec3 halfway_dir = normalize(frag_to_light + frag_to_view);

    vec3 ambient_component = sun.ambient * frag_albedo * vec3(0.04f);

    float diffuse_strength = max(0.0f, dot(frag_to_light, frag_normal));
    vec3 diffuse_component = sun.diffuse * diffuse_strength * frag_albedo;

    float specular_strength = pow(max(0.0f, dot(halfway_dir, frag_normal)), shininess);
    vec3 specular_component = sun.specular * (specular_strength * frag_specular);

    return (ambient_component + diffuse_component + specular_component);
}

void
main()
{
    // TODO: Correctly implement this shader.
    vec3 frag_pos = texture(texture_position, vs_out.frag_tex_coords).rgb;
    vec3 frag_normal = texture(texture_normal, vs_out.frag_tex_coords).rgb;
    vec3 frag_albedo = texture(texture_albedo_specular, vs_out.frag_tex_coords).rgb;
    float frag_specular = texture(texture_albedo_specular, vs_out.frag_tex_coords).a;

    vec3 sun_contribution = calc_directional_light(sun, frag_albedo, frag_specular, frag_normal);
    vec3 color = sun_contribution;

    color = apply_gamma_correction(color);
    // Set the final color
    frag_color = vec4(color, 1.0);
}

#endif
