#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

class Shader;
struct AsteroidExplosion;

class ParticleSystem {
public:
    static constexpr int   MAX_PARTICLES = 500;
    static constexpr float SPAWN_RATE    = 100.f;

    float spawnAccum = 0.f; // accumulated fractional particles to spawn next frame

    // Allocates GPU buffers.
    void init();
    // Emits one particle.
    void add(glm::vec3 pos, glm::vec3 vel, float maxLife, float size);
    // Emits a burst of particles.
    void spawnExplosion(const AsteroidExplosion& exp);
    bool full()  const { return (int)particles.size() >= MAX_PARTICLES; }
    int  count() const { return (int)particles.size(); }
    // Advances all particles by dt and removes dead ones.
    void update(float dt);
    // Uploads per-particle data and draws all particles as instanced camera-facing quads.
    void draw(const glm::mat4& view, const glm::mat4& projection, Shader& shader);
    // Releases GPU buffers.
    void free();

private:
    struct Particle {
        glm::vec3 pos, vel;
        float     life, maxLife, size;
    };

    std::vector<Particle> particles;
    GLuint ptVAO = 0, ptQuadVBO = 0, ptVBO = 0;
};
