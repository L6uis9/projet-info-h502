#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

class Shader;

class ParticleSystem {
public:
    static constexpr int   MAX_PARTICLES = 500;
    static constexpr float SPAWN_RATE    = 100.f;

    float spawnAccum = 0.f;

    void init();
    void add(glm::vec3 pos, glm::vec3 vel, float maxLife, float size);
    bool full()  const { return (int)particles.size() >= MAX_PARTICLES; }
    int  count() const { return (int)particles.size(); }
    void update(float dt);
    void draw(const glm::mat4& view, const glm::mat4& projection, Shader& shader);
    void free();

private:
    struct Particle {
        glm::vec3 pos, vel;
        float     life, maxLife, size;
    };

    std::vector<Particle> particles;
    GLuint ptVAO = 0, ptVBO = 0;
};
