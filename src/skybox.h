#pragma once
#include <glad/glad.h>
#include <string>
#include <vector>

class Skybox
{
public:
    // faces order: px, nx, py, ny, pz, nz  (right,left,top,bottom,front,back)
    void load(const std::vector<std::string>& faces);
    void draw() const;
    void free();

    GLuint cubemapTexture = 0;

private:
    GLuint VAO = 0, VBO = 0;
};