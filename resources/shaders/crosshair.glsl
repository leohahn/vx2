/* ====================================
 *
 *   Vertex Shader
 *
 * ==================================== */
#ifdef COMPILING_VERTEX

layout (location = 0) in vec3 att_position;
layout (location = 1) in vec3 att_tex_coords_layer;

out VS_OUT
{
    vec3 frag_tex_coords_layer;
} vs_out;

void
main()
{
    vs_out.frag_tex_coords_layer = att_tex_coords_layer;
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
    vec3 frag_tex_coords_layer;
} vs_out;

out vec4 frag_color;

uniform sampler2DArray texture_array;

#define NUM_LAYERS 9

vec3
apply_gamma_correction(vec3 color)
{
    const float gamma = 2.2;
    return pow(color, vec3(1.0/gamma));
}

void
main()
{
    float layer = max(0, min(NUM_LAYERS-1, floor(vs_out.frag_tex_coords_layer.z + 0.5)));
    frag_color.rgb = vec3(1.0, 0.0, 0.0);
    frag_color.a = texture(texture_array, vec3(vs_out.frag_tex_coords_layer.xy, layer)).a;
}

#endif
