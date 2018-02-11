/* ====================================
 *
 *   Vertex Shader
 *
 * ==================================== */
#ifdef COMPILING_VERTEX

layout (location = 0) in vec3 att_position;
layout (location = 1) in vec2 att_tex_coords;
layout (location = 2) in vec3 att_normal;

uniform mat4 projection;
uniform mat4 view;

out VS_OUT
{
    vec3 frag_world_pos;
    vec2 frag_tex_coords;
    vec3 frag_normal;
} vs_out;

void
main()
{
    vs_out.frag_world_pos = att_position;
    vs_out.frag_tex_coords = att_tex_coords;
    vs_out.frag_normal = att_normal;

    gl_Position = projection * view * vec4(att_position, 1.0f);
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
    vec2 frag_tex_coords;
    vec3 frag_normal;
} vs_out;

layout (location = 0) out vec3 gbuffer_position;
layout (location = 1) out vec3 gbuffer_normal;
layout (location = 2) out vec4 gbuffer_albedo_specular;

void
main()
{
    vec3 color = vec3(0.0, 1.0, 0.0); // TODO: take this color from a texture

    gbuffer_position = vs_out.frag_world_pos;
    gbuffer_normal = normalize(vs_out.frag_normal);
    gbuffer_albedo_specular.rgb = color;
    gbuffer_albedo_specular.a = 1.0; // TODO: get this from specular texture maybe?
}

#endif
