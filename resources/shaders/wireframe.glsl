/* ====================================
 *
 *   Vertex Shader
 *
 * ==================================== */
#ifdef COMPILING_VERTEX

layout (location = 0) in vec3 att_position;

uniform mat4 projection;
uniform mat4 view;

void
main()
{
    gl_Position = projection * view * vec4(att_position, 1.0f);
}

#endif

/* ====================================
 *
 *   Fragment Shader
 *
 * ==================================== */
#ifdef COMPILING_FRAGMENT

out vec4 frag_color;

void
main()
{
    frag_color = vec4(1.0);
}

#endif
