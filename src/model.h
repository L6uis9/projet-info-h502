#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
};

struct Material {
    std::string name;
    glm::vec3   Kd  = {0.8f, 0.8f, 0.8f};
    GLuint      diffuseTexture = 0;
    bool        hasDiffuse     = false;
};

struct Mesh {
    std::vector<Vertex>   vertices;
    std::vector<uint32_t> indices;
    Material              material;

    GLuint VAO = 0, VBO = 0, EBO = 0;

    void upload();
    void draw() const;
    void free();
};

class Model
{
public:
    std::vector<Mesh> meshes;

    // objPath   : path to the .obj file
    // texDir    : directory that contains the textures referenced in the .mtl file
    void load(const std::string& objPath, const std::string& texDir);
    void draw() const;
    void free();
};