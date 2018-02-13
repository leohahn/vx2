#ifndef __SHADER_HPP__
#define __SHADER_HPP__

#include <functional>
#include <unordered_map>

#include "lt_core.hpp"
#include "lt_math.hpp"

struct Shader
{
    std::string filepath;
    u32 program = 0;

    explicit Shader(const std::string &filepath);
    ~Shader();

    void load();

    void setup_projection_matrix(f32 aspect_ratio);
    void set3f(const char *name, Vec3f v);
    void set1i(const char *name, i32 i);
    void set1f(const char *name, f32 f);
    void set_matrix(const char *name, const Mat4f &m);

    void add_texture(const char *name);
    u32 texture_unit(const char *name) const;
    u32 texture_unit(const std::string &name) const;

    void use() const;

private:
    i32 m_next_texture_unit;
    std::unordered_map<std::string, u32> m_locations;
    std::unordered_map<std::string, u32> m_texture_units;

    u32 get_location(const char *name);
};

#endif // __SHADER_HPP__
