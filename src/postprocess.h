#pragma once
#include <glad/glad.h>
#include "shader.h"

class PostProcess {
public:
    void init(int w, int h);
    void begin();
    void apply(Shader& shader, float blurStrength);
    void free();

private:
    GLuint fbo_ = 0, colorTex_ = 0, depthRBO_ = 0;
    GLuint quadVAO_ = 0, quadVBO_ = 0, quadEBO_ = 0;
    int w_ = 0, h_ = 0;
};
