#include "shader.hpp"

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <unordered_map>

#include "glad/glad.h"
#include "resources.hpp"
#include "lt_fs.hpp"
#include "lt_utils.hpp"
#include "camera.hpp"
#include "resource_manager.hpp"
#include "application.hpp"

lt_internal lt::Logger logger("shader");

lt_internal GLuint
make_program(const std::string &path)
{
    using std::string;

    logger.log("Making shader program for ", path);

    // Fetch source codes from each shader
    FileContents *shader_src = file_read_contents(path.c_str());

    LT_Assert(shader_src != nullptr);

    if (shader_src->error != FileError_None)
    {
        logger.error("Error reading shader source from ", path);
        file_free_contents(shader_src);
        return 0;
    }

    string shader_string((char*)shader_src->data, (char*)shader_src->data + shader_src->size - 1);

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

    if (vertex_shader == 0 || fragment_shader == 0)
    {
        logger.error("Creating shaders (glCreateShader)\n");
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
        file_free_contents(shader_src);
        return 0;
    }

    GLuint program = 0;
    GLchar info[512] = {};
    GLint success;
    {
        const char *vertex_string[3] = {
            "#version 330 core\n",
            "#define COMPILING_VERTEX\n",
            shader_string.c_str(),
        };
        glShaderSource(vertex_shader, 3, &vertex_string[0], NULL);

        const char *fragment_string[3] = {
            "#version 330 core\n",
            "#define COMPILING_FRAGMENT\n",
            shader_string.c_str(),
        };
        glShaderSource(fragment_shader, 3, &fragment_string[0], NULL);
    }

    glCompileShader(vertex_shader);
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        glGetShaderInfoLog(vertex_shader, 512, NULL, info);
        logger.error("Vertex shader compilation failed:");
        printf("%s\n", info);
        goto error_cleanup;
    }

    glCompileShader(fragment_shader);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        glGetShaderInfoLog(fragment_shader, 512, NULL, info);
        logger.error("Fragment shader compilation failed:");
        printf("%s\n", info);
        goto error_cleanup;
    }

    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success)
    {
        glGetShaderInfoLog(fragment_shader, 512, nullptr, info);
        logger.error("Shader linking failed:");
        printf("%s\n", info);
        goto error_cleanup;
    }

    file_free_contents(shader_src);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    return program;

error_cleanup:
    file_free_contents(shader_src);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    return 0;
}

Shader::Shader(const std::string &filepath)
    : filepath(filepath)
    , m_next_texture_unit(0)
{
}

Shader::~Shader()
{
    glDeleteProgram(program);
}

void
Shader::load()
{
    program = make_program(filepath);
}

void
Shader::add_texture(const char *name)
{
    std::string str_name(name);
    LT_Assert(m_texture_units.find(str_name) == m_texture_units.end());

    m_texture_units[str_name] = m_next_texture_unit;

    if (program)
    {
        glUseProgram(program);
        set1i(name, m_next_texture_unit);
        glUseProgram(0);
    }
    else
        logger.error("Shader ", filepath, " is not yet loaded.");

    m_next_texture_unit++;

    dump_opengl_errors("add_texture", __FILE__);
}

u32
Shader::texture_unit(const char *name) const
{
    std::string str_name(name);
    LT_Assert(m_texture_units.find(str_name) != m_texture_units.end());
    return m_texture_units.at(str_name);
}

u32
Shader::texture_unit(const std::string &name) const
{
    return texture_unit(name.c_str());
}

void
Shader::setup_perspective_matrix(f32 aspect_ratio)
{
    const Mat4f projection = lt::perspective(60.0f, aspect_ratio, Camera::ZNEAR, Camera::ZFAR);
    glUseProgram(program);
    glUniformMatrix4fv(get_location("projection"), 1, GL_FALSE, projection.data());
}

void
Shader::setup_orthographic_matrix(f32 left, f32 right, f32 bottom, f32 top)
{
    const Mat4f projection = lt::orthographic(left, right, bottom, top);
    glUseProgram(program);
    glUniformMatrix4fv(get_location("projection"), 1, GL_FALSE, projection.data());
}

void
Shader::set3f(const char *name, Vec3f v)
{
    glUniform3f(get_location(name), v.x, v.y, v.z);
}

void Shader::use() const { glUseProgram(program); }

void
Shader::set1i(const char *name, i32 i)
{
    glUniform1i(get_location(name), i);
}

void
Shader::set1f(const char *name, f32 f)
{
    glUniform1f(get_location(name), f);
}

void
Shader::set_matrix(const char *name, const Mat4f &m)
{
    glUniformMatrix4fv(get_location(name), 1, GL_FALSE, m.data());
}

GLuint
Shader::get_location(const char *name)
{
    if (m_locations.find(name) == m_locations.end())
    {
        const GLuint location = glGetUniformLocation(program, name);
        m_locations[std::string(name)] = location;
    }

    return m_locations.at(name);
}
