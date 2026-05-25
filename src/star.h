#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

class Shader;

struct StarEntry {
    glm::vec3 dir;
    glm::vec3 color;
    float     radius;
};

class Star {
public:
    // Allocates the billboard quad geometry on the GPU.
    void init();
    // Renders each star as a camera-facing billboard positioned at infinity.
    void draw(Shader& shader, const glm::mat4& view, const glm::mat4& projection,
              const std::vector<StarEntry>& suns) const;
    // Releases GPU resources.
    void free();

private:
    GLuint VAO_ = 0, VBO_ = 0, EBO_ = 0;
};
