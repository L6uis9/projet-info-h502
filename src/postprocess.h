#pragma once
#include <glad/glad.h>
#include "shader.h"

class PostProcess {
public:
    // Creates the FBO (color texture + depth renderbuffer) and the full-screen quad geometry.
    void init(int w, int h);
    void begin();                                     // bind FBO call before drawing the scene
    void apply(Shader& shader, float blurStrength);   // transfer FBO to screen with post-processing
    // Releases GPU resources.
    void free();

private:
    GLuint fbo_ = 0, colorTex_ = 0, depthRBO_ = 0;
    GLuint quadVAO_ = 0, quadVBO_ = 0, quadEBO_ = 0;
    int w_ = 0, h_ = 0;
};
