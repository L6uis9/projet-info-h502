#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

class Shader
{
public:
    GLuint ID = 0;

    Shader() = default;
    Shader(const std::string& vertPath, const std::string& fragPath);

    void use() const;

    // Uniform setters
    void setBool (const std::string& name, bool  v) const;
    void setInt  (const std::string& name, int   v) const;
    void setFloat(const std::string& name, float v) const;
    void setVec3 (const std::string& name, const glm::vec3& v) const;
    void setMat4 (const std::string& name, const glm::mat4& v) const;

private:
    static GLuint compile(GLenum type, const std::string& src);
    static std::string readFile(const std::string& path);
};