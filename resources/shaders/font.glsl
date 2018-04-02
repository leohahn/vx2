/* ====================================
 *
 *   Vertex Shader
 *
 * ==================================== */
#ifdef COMPILING_VERTEX

layout (location = 0) in vec3 att_position;
layout (location = 1) in vec2 att_tex_coords;
layout (location = 2) in vec3 att_color;

out VS_OUT
{
    vec2 frag_tex_coords;
    vec3 color;
} vs_out;

uniform mat4 projection;

void
main()
{
    vs_out.frag_tex_coords = att_tex_coords;
    vs_out.color = att_color;
    gl_Position = projection * vec4(att_position, 1.0f);
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
    vec3 color;
} vs_out;

out vec4 frag_color;

uniform sampler2D font_atlas;

void
main()
{
    frag_color = vec4(vs_out.color, texture(font_atlas, vs_out.frag_tex_coords).r);
}

#endif
