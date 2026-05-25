#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

class Shader
{
public:
    GLuint ID = 0;

    Shader() = default;
    // Compiles both shader stages, links them into a program.
    Shader(const std::string& vertPath, const std::string& fragPath);

    // Binds this program.
    void use() const;

    // Uniform setters
    void setBool (const std::string& name, bool  v) const;
    void setInt  (const std::string& name, int   v) const;
    void setFloat(const std::string& name, float v) const;
    void setVec3 (const std::string& name, const glm::vec3& v) const;
    void setMat4 (const std::string& name, const glm::mat4& v) const;

private:
    // Compiles a single GLSL stage.
    static GLuint compile(GLenum type, const std::string& src);
    // Reads a text file into a string.
    static std::string readFile(const std::string& path);
};