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

uniform mat4 projection;

void
main()
{
    vs_out.frag_tex_coords = att_tex_coords;
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
} vs_out;

out vec4 frag_color;

uniform sampler2D font_atlas;
// TODO: maybe add a uniform for color?

void
main()
{
    // frag_color = vec4(1.0);
    // float r = texture(font_atlas, vs_out.frag_tex_coords).r;
    // frag_color = vec4(r, r, r, 1.0);
    // frag_color = vec4(0.0, 0.0, 1.0, texture(font_atlas, vs_out.frag_tex_coords).r);
    frag_color = vec4(0.561, 0.706, 0.937, texture(font_atlas, vs_out.frag_tex_coords).r);
}

#endif
