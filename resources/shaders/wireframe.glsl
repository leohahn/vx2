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

layout (location = 0) out vec3 gbuffer_position;
layout (location = 1) out vec3 gbuffer_normal;
layout (location = 2) out vec4 gbuffer_albedo_specular;

void
main()
{
    gbuffer_albedo_specular.rgb = vec3(1.0);
}

#endif
