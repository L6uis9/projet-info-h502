#include "particle.h"
#include "shader.h"
#include "asteroid.h"
#include "utils.h"

#include <algorithm>
#include <glm/glm.hpp>

void ParticleSystem::init()
{
    particles.reserve(MAX_PARTICLES);

    static const float quadVerts[18] = {
        -1.f, -1.f, 0.f,   1.f, -1.f, 0.f,  -1.f,  1.f, 0.f,
         1.f, -1.f, 0.f,   1.f,  1.f, 0.f,  -1.f,  1.f, 0.f
    };

    glGenVertexArrays(1, &ptVAO);
    glGenBuffers(1, &ptQuadVBO);
    glGenBuffers(1, &ptVBO);
    glBindVertexArray(ptVAO);

    // Quad corners
    glBindBuffer(GL_ARRAY_BUFFER, ptQuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glVertexAttribDivisor(0, 0);

    // particle i:  px py pz sz  r g b a (center + size, color)
    glBindBuffer(GL_ARRAY_BUFFER, ptVBO);
    glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * 8 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glVertexAttribDivisor(1, 1);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(4 * sizeof(float)));
    glVertexAttribDivisor(2, 1);

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
        float sz = p.size * t * 0.05f;  // converts size to world units
        // Layout: center(xyz + w=size), col(rgba)
        gpuBuf.push_back(p.pos.x); gpuBuf.push_back(p.pos.y); gpuBuf.push_back(p.pos.z);
        gpuBuf.push_back(sz);
        gpuBuf.push_back(col.r);   gpuBuf.push_back(col.g);   gpuBuf.push_back(col.b);
        gpuBuf.push_back(alpha);
    }

    glBindBuffer(GL_ARRAY_BUFFER, ptVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    (GLsizeiptr)(gpuBuf.size() * sizeof(float)), gpuBuf.data());

    glm::vec3 cameraRight(view[0][0], view[1][0], view[2][0]);
    glm::vec3 cameraUp   (view[0][1], view[1][1], view[2][1]);

    shader.use();
    shader.setMat4("V",           view);
    shader.setMat4("P",           projection);
    shader.setVec3("cameraRight", cameraRight);
    shader.setVec3("cameraUp",    cameraUp);

    glDepthMask(GL_FALSE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glBindVertexArray(ptVAO);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, (GLsizei)particles.size());
    glBindVertexArray(0);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);
}

void ParticleSystem::spawnExplosion(const AsteroidExplosion& exp)
{
    int count = 15 + (int)(exp.scale * 5.f);
    for (int k = 0; k < count && !full(); k++)
        add(exp.center + randUnitVec() * exp.scale * 0.4f,
            randUnitVec() * randF(4.f, 18.f),
            randF(0.5f, 1.2f), randF(8.f, 18.f));
}

void ParticleSystem::free()
{
    glDeleteVertexArrays(1, &ptVAO);
    glDeleteBuffers(1, &ptQuadVBO);
    glDeleteBuffers(1, &ptVBO);
}
