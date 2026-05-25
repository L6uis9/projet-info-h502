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
    bool        refractive     = false;  // true when MTL dissolve (d) < 1
    float       Ni             = 1.0f;   // index of refraction from MTL "Ni" field
};

struct Mesh {
    std::vector<Vertex>   vertices;
    std::vector<uint32_t> indices;
    Material              material;

    GLuint VAO = 0, VBO = 0, EBO = 0;

    // Uploads vertex and index data to GPU (VBO + EBO). Call once after filling vertices/indices.
    void upload();
    // Issues a glDrawElements call for this mesh.
    void draw() const;
    // Releases GPU buffers.
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