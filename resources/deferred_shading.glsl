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
} vs_out;

void
main()
{
    vs_out.frag_tex_coords = att_tex_coords;
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
} vs_out;

out vec4 frag_color;

uniform sampler2D texture_position;
uniform sampler2D texture_normal;
uniform sampler2D texture_albedo_specular;

vec3
apply_gamma_correction(vec3 color)
{
    const float gamma = 2.2;
    return pow(color, vec3(1.0/gamma));
}

void
main()
{
    // TODO: Correctly implement this shader.
    vec3 frag_pos = texture(texture_position, vs_out.frag_tex_coords).rgb;
    vec3 frag_normal = texture(texture_normal, vs_out.frag_tex_coords).rgb;
    vec3 frag_albedo = texture(texture_albedo_specular, vs_out.frag_tex_coords).rgb;
    float frag_specular = texture(texture_albedo_specular, vs_out.frag_tex_coords).a;

    vec3 color = frag_albedo;

    color = apply_gamma_correction(color);
    // Set the final color
    frag_color = vec4(color, 1.0);
}

#endif
