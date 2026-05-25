#pragma once
#include <glad/glad.h>
#include <string>
#include <vector>

class Skybox
{
public:
    // Loads the six cubemap faces and uploads the unit cube geometry to the GPU.
    // faces order: px, nx, py, ny, pz, nz  (right,left,top,bottom,front,back)
    void load(const std::vector<std::string>& faces);
    // Draws the skybox cube.
    void draw() const;
    // Releases GPU resources.
    void free();

    GLuint cubemapTexture = 0;

private:
    GLuint VAO = 0, VBO = 0;
};