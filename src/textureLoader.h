#pragma once
#include <glad/glad.h>
#include <string>
#include <vector>

// Load a 2-D texture from disk (uses stb_image)
GLuint loadTexture(const std::string& path);

// Load a cubemap from 6 face images in the order:
//   right (+X), left (-X), top (+Y), bottom (-Y), front (+Z), back (-Z)
GLuint loadCubemap(const std::vector<std::string>& faces);