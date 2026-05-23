#include "star.h"
#include "shader.h"

void Star::init()
{   
    // Sun billboard quad
    const float quad[] = { -0.5f,-0.5f,  0.5f,-0.5f,  0.5f, 0.5f,  -0.5f, 0.5f };
    const unsigned int idx[] = { 0,1,2, 0,2,3 };

    glGenVertexArrays(1, &VAO_);
    glGenBuffers(1, &VBO_);
    glGenBuffers(1, &EBO_);
    glBindVertexArray(VAO_);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindVertexArray(0);
}

void Star::draw(Shader& shader, const glm::mat4& view, const glm::mat4& projection,
                        const std::vector<StarEntry>& suns) const
{
    glDepthFunc(GL_LEQUAL);
    shader.use();
    shader.setMat4("view",       view);
    shader.setMat4("projection", projection);
    glBindVertexArray(VAO_);
    for (auto& s : suns) {
        shader.setVec3 ("sunDir",    s.dir);
        shader.setFloat("sunRadius", s.radius);
        shader.setVec3 ("starColor", s.color);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }
    glBindVertexArray(0);
    glDepthFunc(GL_LESS);
}

void Star::free()
{
    glDeleteVertexArrays(1, &VAO_);
    glDeleteBuffers(1, &VBO_);
    glDeleteBuffers(1, &EBO_);
}
