/* ====================================
 *
 *   Vertex Shader
 *
 * ==================================== */
#ifdef COMPILING_VERTEX

layout (location = 0) in vec3 att_position;
layout (location = 1) in vec2 att_tex_coords;

out vec2 tex_coords;

void
main()
{
    tex_coords = att_tex_coords;
    gl_Position = vec4(att_position, 1.0);
}

#endif

/* ====================================
 *
 *   Fragment Shader
 *
 * ==================================== */
#ifdef COMPILING_FRAGMENT

in vec2 tex_coords;
out vec4 frag_color;

uniform sampler2D texture_shadow_map;

void
main()
{
    float depth = texture(texture_shadow_map, tex_coords).r;
    frag_color = vec4(vec3(depth), 1.0);
}

#endif
