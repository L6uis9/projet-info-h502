#include "shader.h"
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>

std::string Shader::readFile(const std::string& path)
{
    std::ifstream f(path);
    if (!f.is_open())
        throw std::runtime_error("Shader::readFile – cannot open: " + path);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

GLuint Shader::compile(GLenum type, const std::string& src)
{
    GLuint id = glCreateShader(type);
    const char* cstr = src.c_str();
    glShaderSource(id, 1, &cstr, nullptr);
    glCompileShader(id);

    GLint ok = 0;
    glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(id, sizeof(log), nullptr, log);
        glDeleteShader(id);
        throw std::runtime_error(std::string("Shader compile error:\n") + log);
    }
    return id;
}

Shader::Shader(const std::string& vertPath, const std::string& fragPath)
{
    GLuint vert = compile(GL_VERTEX_SHADER,   readFile(vertPath));
    GLuint frag = compile(GL_FRAGMENT_SHADER, readFile(fragPath));

    ID = glCreateProgram();
    glAttachShader(ID, vert);
    glAttachShader(ID, frag);
    glLinkProgram(ID);

    GLint ok = 0;
    glGetProgramiv(ID, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(ID, sizeof(log), nullptr, log);
        glDeleteProgram(ID);
        throw std::runtime_error(std::string("Shader link error:\n") + log);
    }

    glDeleteShader(vert);
    glDeleteShader(frag);
}

void Shader::use() const { glUseProgram(ID); }

void Shader::setBool (const std::string& n, bool  v) const { glUniform1i (glGetUniformLocation(ID, n.c_str()), (int)v); }
void Shader::setInt  (const std::string& n, int   v) const { glUniform1i (glGetUniformLocation(ID, n.c_str()), v); }
void Shader::setFloat(const std::string& n, float v) const { glUniform1f (glGetUniformLocation(ID, n.c_str()), v); }
void Shader::setVec3 (const std::string& n, const glm::vec3& v) const { glUniform3fv(glGetUniformLocation(ID, n.c_str()), 1, glm::value_ptr(v)); }
void Shader::setMat4 (const std::string& n, const glm::mat4& v) const { glUniformMatrix4fv(glGetUniformLocation(ID, n.c_str()), 1, GL_FALSE, glm::value_ptr(v)); }