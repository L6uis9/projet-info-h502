#pragma once
#include "model.h"
#include <glm/glm.hpp>
#include <vector>

class Shader;

class Asteroid {
public:
    Mesh mesh;

    // seed     : changes the noise offset so every asteroid looks different
    // rings    : latitude subdivisions of the base sphere
    // sectors  : longitude subdivisions of the base sphere
    void generate(int seed = 0, int rings = 32, int sectors = 32);
    void free() { mesh.free(); }
};

struct AsteroidInstance {
    int       meshIdx;
    glm::vec3 position;
    glm::vec3 velocity;
    float     scale;
    float     rotSpeed;
    glm::vec3 rotAxis;
    float     rotAngle;
};

struct AsteroidSpawnCfg {
    int   poolSize;
    float minSpawnRadius;
    float maxSpawnRadius;
    float despawnDist;
    float minScale;
    float maxScale;
    float minSpeed;
    float maxSpeed;
    float spawnInterval;
    int   maxCount;
    float towardShipProba;
};

struct AsteroidExplosion {
    glm::vec3 center;
    float     scale;
};

class AsteroidSystem {
public:
    std::vector<Asteroid>         pool;
    std::vector<AsteroidInstance> instances;
    float                         nextSpawnTime = 0.f;

    // Pre-generates the mesh pool and binds the shared asteroid texture.
    void init(const AsteroidSpawnCfg& cfg, GLuint texture);
    // Spawns one asteroid if the spawn timer has elapsed and the cap is not reached.
    void trySpawn(float currentFrame, glm::vec3 shipPos);

    // Moves instances, despawns distant ones, resolves asteroid-asteroid collisions.
    // Returns one AsteroidExplosion per destroyed asteroid.
    std::vector<AsteroidExplosion> update(float dt, glm::vec3 shipPos);

    // Returns true if ship was hit; populates `out` with one entry per destroyed asteroid.
    bool checkShipCollision(glm::vec3 shipPos, float shipRadius,
                            float currentFrame, float& invincibleUntil,
                            float invincibilityDur,
                            std::vector<AsteroidExplosion>& out);

    // Renders all active instances.
    void draw(Shader& shader, float currentFrame) const;
    // Releases GPU resources for every mesh in the pool.
    void free();

private:
    AsteroidSpawnCfg cfg_;
};
