#include "particle.h"
#include "shader.h"

#include <algorithm>
#include <glm/glm.hpp>

void ParticleSystem::init()
{
    particles.reserve(MAX_PARTICLES);

    glGenVertexArrays(1, &ptVAO);
    glGenBuffers(1, &ptVBO);
    glBindVertexArray(ptVAO);
    glBindBuffer(GL_ARRAY_BUFFER, ptVBO);
    glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * 8 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(7 * sizeof(float)));
    glBindVertexArray(0);
}

void ParticleSystem::add(glm::vec3 pos, glm::vec3 vel, float maxLife, float size)
{
    if (full()) return;
    particles.push_back({ pos, vel, maxLife, maxLife, size });
}

void ParticleSystem::update(float dt)
{
    for (auto& p : particles) {
        p.pos  += p.vel * dt;
        p.vel  *= (1.f - 3.f * dt);
        p.life -= dt;
    }
    particles.erase(
        std::remove_if(particles.begin(), particles.end(),
            [](const Particle& p){ return p.life <= 0.f; }),
        particles.end());
}

void ParticleSystem::draw(const glm::mat4& view, const glm::mat4& projection, Shader& shader)
{
    if (particles.empty()) return;

    std::vector<float> gpuBuf;
    gpuBuf.reserve(particles.size() * 8);
    for (auto& p : particles) {
        float t = p.life / p.maxLife;
        glm::vec3 col;
        float alpha;
        if (t > 0.5f) {
            float u = (t - 0.5f) * 2.f;
            col   = glm::mix(glm::vec3(1.f, 0.4f, 0.f), glm::vec3(1.f, 0.92f, 0.4f), u);
            alpha = glm::mix(0.5f, 0.65f, u);
        } else {
            float u = t * 2.f;
            col   = glm::mix(glm::vec3(0.25f, 0.f, 0.f), glm::vec3(1.f, 0.4f, 0.f), u);
            alpha = u * 0.5f;
        }
        float sz = p.size * t;
        gpuBuf.push_back(p.pos.x); gpuBuf.push_back(p.pos.y); gpuBuf.push_back(p.pos.z);
        gpuBuf.push_back(col.r);   gpuBuf.push_back(col.g);   gpuBuf.push_back(col.b);
        gpuBuf.push_back(alpha);
        gpuBuf.push_back(sz);
    }

    glBindBuffer(GL_ARRAY_BUFFER, ptVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    (GLsizeiptr)(gpuBuf.size() * sizeof(float)), gpuBuf.data());

    shader.use();
    shader.setMat4("view",       view);
    shader.setMat4("projection", projection);

    glDepthMask(GL_FALSE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glBindVertexArray(ptVAO);
    glDrawArrays(GL_POINTS, 0, (GLsizei)particles.size());
    glBindVertexArray(0);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);
}

void ParticleSystem::free()
{
    glDeleteVertexArrays(1, &ptVAO);
    glDeleteBuffers(1, &ptVBO);
}
