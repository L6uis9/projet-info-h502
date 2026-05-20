#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include "shader.h"
#include "model.h"
#include "skybox.h"
#include "camera.h"
#include "asteroid.h"
#include "textureLoader.h"

// Window settings
static const int   SCR_W  = 1280;
static const int   SCR_H  = 720;
static const char* TITLE  = "Space Game";

// Asteroid system settings
static const int   ASTEROID_MESH_POOL      = 25;    // distinct pre-generated shapes
static const float ASTEROID_MIN_SPAWN_RADIUS = 100.f;  // minimum spawn distance from ship
static const float ASTEROID_SPAWN_RADIUS     = 200.f; // maximum spawn distance from ship
static const float ASTEROID_DESPAWN_DIST   = 170.f; // remove when farther than this from ship
static const float ASTEROID_MIN_SCALE      = 0.1f;
static const float ASTEROID_MAX_SCALE      = 4.5f;
static const float ASTEROID_MIN_SPEED      = 40.f;
static const float ASTEROID_MAX_SPEED      = 60.f;
static const float ASTEROID_SPAWN_INTERVAL = 0.01f;  // seconds between spawns
static const int   ASTEROID_MAX_COUNT      = 200;
static const float ASTEROID_TOWARD_SHIP_PROBA = 0.2f;

// Ship collision settings
static const float SHIP_RADIUS       = 1.5f;  // approximate sphere radius
static const float SHIP_INVINCIBILITY = 2.0f; // seconds of invincibility after being hit

// Ship physics settings
static const float SHIP_ACCEL = 50.f;
static const float SHIP_DRAG  = 0.5f;

// Globals
static Camera     camera;
static glm::vec3  shipPos      = {0.f, 0.f, 0.f};
static glm::vec3  shipVelocity = {0.f, 0.f, 0.f};
static float      lastX      = SCR_W / 2.f;
static float      lastY      = SCR_H / 2.f;
static bool       firstMouse = true;
static float      deltaTime  = 0.f;
static float      lastFrame  = 0.f;
static bool       lookBack   = false;
static float      shipInvincibleUntil = 0.f;

// GLFW callbacks

static void framebuffer_size_cb(GLFWwindow*, int w, int h)
{
    glViewport(0, 0, w, h);
}

static void cursor_pos_cb(GLFWwindow*, double xpos, double ypos)
{
    if (firstMouse) {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }
    float xOff = (float)xpos - lastX;
    float yOff = lastY - (float)ypos; // reversed Y
    lastX = (float)xpos;
    lastY = (float)ypos;
    camera.processMouse(xOff, yOff);
}

static void scroll_cb(GLFWwindow*, double, double yoffset)
{
    camera.fov -= (float)yoffset;
    if (camera.fov <  1.f) camera.fov =  1.f;
    if (camera.fov > 90.f) camera.fov = 90.f;
}

static void key_cb(GLFWwindow* win, int key, int, int action, int)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(win, true);
}

// Continuous key handling – updates ship velocity, then snaps camera behind it
static void processInput(GLFWwindow* win)
{
    glm::vec3 fwd = camera.front;

    if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(win, GLFW_KEY_Z) == GLFW_PRESS)
        shipVelocity += fwd * SHIP_ACCEL * deltaTime;

    shipVelocity *= std::max(0.f, 1.f - SHIP_DRAG * deltaTime);

    lookBack = (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS);

    float dist = 8.f;
    float side  = lookBack ? 1.f : -1.f;
    camera.position = shipPos + fwd * dist * side + glm::vec3(0.f, 1.5f, 0.f);
}

int main()
{
    // Init GLFW
    if (!glfwInit())
        throw std::runtime_error("glfwInit failed");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_W, SCR_H, TITLE, nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        throw std::runtime_error("glfwCreateWindow failed");
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_cb);
    glfwSetCursorPosCallback(window,       cursor_pos_cb);
    glfwSetScrollCallback(window,          scroll_cb);
    glfwSetKeyCallback(window,             key_cb);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // capture mouse

    // Init GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        throw std::runtime_error("gladLoadGLLoader failed");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_PROGRAM_POINT_SIZE);

    // Shaders
    Shader modelShader   ("shaders/model.vert",    "shaders/model.frag");
    Shader skyboxShader  ("shaders/skybox.vert",   "shaders/skybox.frag");
    Shader starShader    ("shaders/star.vert",     "shaders/star.frag");
    Shader particleShader("shaders/particle.vert", "shaders/particle.frag");

    // Skybox
    // px=right, nx=left, py=top, ny=bottom, pz=front, nz=back
    Skybox skybox;
    skybox.load({
        "assets/cubemap/px.png",
        "assets/cubemap/nx.png",
        "assets/cubemap/py.png",
        "assets/cubemap/ny.png",
        "assets/cubemap/pz.png",
        "assets/cubemap/nz.png"
    });

    // Spaceship model
    Model spaceship;
    spaceship.load("assets/models/spaceship.obj",
                   "assets/textures/spaceship");

    // Asteroid texture and mesh pool (pre-generated to avoid runtime cost)
    GLuint asteroidTex = loadTexture("assets/textures/asteroid/photo-stone-texture-pattern.jpg");

    std::vector<Asteroid> asteroidPool(ASTEROID_MESH_POOL);
    for (int i = 0; i < ASTEROID_MESH_POOL; i++) {
        asteroidPool[i].generate(i);
        asteroidPool[i].mesh.material.diffuseTexture = asteroidTex;
        asteroidPool[i].mesh.material.hasDiffuse     = true;
    }

    struct AsteroidInstance {
        int       meshIdx;
        glm::vec3 position;
        glm::vec3 velocity;
        float     scale;
        float     rotSpeed;
        glm::vec3 rotAxis;
        float     rotAngle; // initial phase offset (degrees)
    };

    std::vector<AsteroidInstance> asteroids;
    float nextSpawnTime = 0.f;

    srand(42);

    auto randF = [](float lo, float hi) -> float {
        return lo + (float)rand() / RAND_MAX * (hi - lo);
    };
    auto randUnitVec = [&]() -> glm::vec3 {
        float u     = randF(-1.f, 1.f);
        float theta = randF(0.f, 2.f * glm::pi<float>());
        float r     = std::sqrt(std::max(0.f, 1.f - u * u));
        return { r * std::cos(theta), u, r * std::sin(theta) };
    };

    auto spawnAsteroid = [&]() {
        if ((int)asteroids.size() >= ASTEROID_MAX_COUNT) return;

        float spawnDist = randF(ASTEROID_MIN_SPAWN_RADIUS, ASTEROID_SPAWN_RADIUS);
        glm::vec3 spawnPos = shipPos + randUnitVec() * spawnDist;

        glm::vec3 dir = (randF(0.f, 1.f) < ASTEROID_TOWARD_SHIP_PROBA)
                        ? glm::normalize(shipPos - spawnPos)
                        : randUnitVec();

        AsteroidInstance inst;
        inst.meshIdx  = rand() % ASTEROID_MESH_POOL;
        inst.position = spawnPos;
        inst.velocity = dir * randF(ASTEROID_MIN_SPEED, ASTEROID_MAX_SPEED);
        inst.scale    = randF(ASTEROID_MIN_SCALE, ASTEROID_MAX_SCALE);
        inst.rotSpeed = randF(5.f, 30.f);
        inst.rotAxis  = randUnitVec();
        inst.rotAngle = randF(0.f, 360.f);
        asteroids.push_back(inst);
    };

    // Pre-configure skybox shader sampler
    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    // All stars: single source of truth for both lighting and visual rendering
    struct VisualStar { glm::vec3 dir; glm::vec3 color; float radius; };
    const std::vector<VisualStar> visualStars = {
        { glm::normalize(glm::vec3( 4000.f,  1500.f, -2000.f)), glm::vec3(1.00f, 0.80f, 0.50f), 0.18f }, // main sun
        { glm::normalize(glm::vec3(-3000.f,   800.f, -1500.f)), glm::vec3(1.00f, 0.85f, 0.15f), 0.07f }, // yellow star
        { glm::normalize(glm::vec3(-2000.f,   600.f,  3000.f)), glm::vec3(0.35f, 0.65f, 1.00f), 0.06f }, // blue star
    };

    // Pre-configure model shader
    modelShader.use();
    modelShader.setInt ("diffuseMap",   0);
    modelShader.setVec3("ambientColor", {0.04f, 0.04f, 0.07f});
    modelShader.setInt ("numStars",     (int)visualStars.size());
    for (int i = 0; i < (int)visualStars.size(); i++) {
        modelShader.setVec3("starDirs["   + std::to_string(i) + "]", visualStars[i].dir);
        modelShader.setVec3("starColors[" + std::to_string(i) + "]", visualStars[i].color);
    }

    // Sun billboard quad
    const float sunQuad[] = { -0.5f,-0.5f,  0.5f,-0.5f,  0.5f, 0.5f,  -0.5f, 0.5f };
    const unsigned int sunIdx[] = { 0,1,2, 0,2,3 };
    GLuint sunVAO, sunVBO, sunEBO;
    glGenVertexArrays(1, &sunVAO);
    glGenBuffers(1, &sunVBO);
    glGenBuffers(1, &sunEBO);
    glBindVertexArray(sunVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sunVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(sunQuad), sunQuad, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sunEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(sunIdx), sunIdx, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    // Fire trail particle system
    struct Particle {
        glm::vec3 pos;
        glm::vec3 vel;
        float     life;     // remaining seconds
        float     maxLife;
        float     size;
    };

    static const int   MAX_PT    = 500;
    static const float PT_RATE   = 100.f; // particles per second

    // Asteroid trail particle system
    static const int   MAX_APT              = 4000;
    static const float ASTEROID_TRAIL_RATE  = 20.f; // particles per second per asteroid

    std::vector<Particle> asteroidParticles;
    asteroidParticles.reserve(MAX_APT);

    std::vector<Particle> particles;
    particles.reserve(MAX_PT);
    float ptSpawnAccum = 0.f;

    // GPU buffer layout: [vec3 pos | vec4 color | float size] = 8 floats
    GLuint ptVAO, ptVBO;
    glGenVertexArrays(1, &ptVAO);
    glGenBuffers(1, &ptVBO);
    glBindVertexArray(ptVAO);
    glBindBuffer(GL_ARRAY_BUFFER, ptVBO);
    glBufferData(GL_ARRAY_BUFFER, MAX_PT * 8 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(7 * sizeof(float)));
    glBindVertexArray(0);

    // Asteroid trail VAO/VBO (same layout as fire trail: vec3 pos | vec4 color | float size)
    GLuint aptVAO, aptVBO;
    glGenVertexArrays(1, &aptVAO);
    glGenBuffers(1, &aptVBO);
    glBindVertexArray(aptVAO);
    glBindBuffer(GL_ARRAY_BUFFER, aptVBO);
    glBufferData(GL_ARRAY_BUFFER, MAX_APT * 8 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(7 * sizeof(float)));
    glBindVertexArray(0);

    // Game loop
    while (!glfwWindowShouldClose(window))
    {
        // Timing
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Input
        glfwPollEvents();
        shipPos += shipVelocity * deltaTime;
        processInput(window);

        // Debug title
        
        {
            char title[128];
            float speed = glm::length(shipVelocity);
            snprintf(title, sizeof(title),
                "Space Game | spd: %.1f  pos: (%.1f, %.1f, %.1f)",
                speed, shipPos.x, shipPos.y, shipPos.z);
            glfwSetWindowTitle(window, title);
        }
        

        // Fire trail: spawn particles when thrusting
        bool thrusting = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS ||
                         glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS;
        if (thrusting) {
            ptSpawnAccum += PT_RATE * deltaTime;
            int toSpawn = (int)ptSpawnAccum;
            ptSpawnAccum -= (float)toSpawn;

            glm::vec3 back  = -camera.front;
            glm::vec3 right = glm::normalize(glm::cross(camera.front, glm::vec3(0.f, 1.f, 0.f)));
            glm::vec3 up    = glm::cross(right, camera.front);

            for (int i = 0; i < toSpawn && (int)particles.size() < MAX_PT; ++i) {
                Particle p;
                p.pos     = shipPos + back * 1.2f
                          + right * randF(-0.45f, 0.45f)
                          + up    * randF(-0.5f, 0.5f);
                p.vel     = back  * randF(8.f, 16.f)
                          + right * randF(-1.5f, 1.5f)
                          + up    * randF(-1.f,  1.f);
                p.maxLife = randF(0.2f, 0.35f);
                p.life    = p.maxLife;
                p.size    = randF(4.f, 8.f);
                particles.push_back(p);
            }
        }

        // Update fire particles
        for (auto& p : particles) {
            p.pos  += p.vel * deltaTime;
            p.vel  *= (1.f - 3.f * deltaTime); // drag
            p.life -= deltaTime;
        }
        particles.erase(
            std::remove_if(particles.begin(), particles.end(),
                [](const Particle& p){ return p.life <= 0.f; }),
            particles.end());

        // Spawn new asteroids
        if (currentFrame >= nextSpawnTime) {
            spawnAsteroid();
            nextSpawnTime = currentFrame + ASTEROID_SPAWN_INTERVAL;
        }

        // Move asteroids and remove those too far from the ship
        for (auto& a : asteroids) {
            a.position += a.velocity * deltaTime;

            // Spawn blue trail particles behind each asteroid
            if ((int)asteroidParticles.size() < MAX_APT &&
                randF(0.f, 1.f) < ASTEROID_TRAIL_RATE * deltaTime)
            {
                float vlen = glm::length(a.velocity);
                if (vlen > 0.01f) {
                    glm::vec3 back = -a.velocity / vlen;
                    glm::vec3 ref  = (std::abs(back.y) < 0.9f) ? glm::vec3(0.f, 1.f, 0.f)
                                                                : glm::vec3(1.f, 0.f, 0.f);
                    glm::vec3 perpA = glm::normalize(glm::cross(back, ref));
                    glm::vec3 perpB = glm::cross(back, perpA);
                    float spread = a.scale * 0.35f;

                    Particle p;
                    p.pos     = a.position + back * a.scale * 0.6f
                              + perpA * randF(-spread, spread)
                              + perpB * randF(-spread, spread);
                    p.vel     = back  * randF(2.f, 7.f)
                              + perpA * randF(-1.f, 1.f)
                              + perpB * randF(-1.f, 1.f);
                    p.maxLife = randF(0.6f, 1.2f);
                    p.life    = p.maxLife;
                    p.size    = randF(6.f, 14.f) * std::min(a.scale, 3.f);
                    asteroidParticles.push_back(p);
                }
            }
        }

        // Update asteroid trail particles
        for (auto& p : asteroidParticles) {
            p.pos  += p.vel * deltaTime;
            p.vel  *= (1.f - 2.f * deltaTime);
            p.life -= deltaTime;
        }
        asteroidParticles.erase(
            std::remove_if(asteroidParticles.begin(), asteroidParticles.end(),
                [](const Particle& p){ return p.life <= 0.f; }),
            asteroidParticles.end());

        asteroids.erase(
            std::remove_if(asteroids.begin(), asteroids.end(),
                [&](const AsteroidInstance& a) {
                    return glm::length(a.position - shipPos) > ASTEROID_DESPAWN_DIST;
                }),
            asteroids.end()
        );

        // Asteroid-asteroid sphere collision detection
        {
            std::vector<bool> explode(asteroids.size(), false);
            for (int i = 0; i < (int)asteroids.size(); i++) {
                for (int j = i + 1; j < (int)asteroids.size(); j++) {
                    float dist    = glm::length(asteroids[i].position - asteroids[j].position);
                    float radSum  = asteroids[i].scale + asteroids[j].scale;
                    if (dist < radSum) {
                        explode[i] = true;
                        explode[j] = true;
                    }
                }
            }
            for (int i = 0; i < (int)asteroids.size(); i++) {
                if (!explode[i]) continue;
                glm::vec3 center = asteroids[i].position;
                int count = 15 + (int)(asteroids[i].scale * 5.f);
                for (int k = 0; k < count && (int)particles.size() < MAX_PT; k++) {
                    Particle p;
                    p.pos     = center + randUnitVec() * asteroids[i].scale * 0.4f;
                    p.vel     = randUnitVec() * randF(4.f, 18.f);
                    p.maxLife = randF(0.5f, 1.2f);
                    p.life    = p.maxLife;
                    p.size    = randF(8.f, 18.f);
                    particles.push_back(p);
                }
            }
            std::vector<AsteroidInstance> survivors;
            survivors.reserve(asteroids.size());
            for (int i = 0; i < (int)asteroids.size(); i++) {
                if (!explode[i]) survivors.push_back(asteroids[i]);
            }
            asteroids = std::move(survivors);
        }

        // Asteroid-ship sphere collision detection
        if (currentFrame > shipInvincibleUntil) {
            std::vector<bool> hitShip(asteroids.size(), false);
            for (int i = 0; i < (int)asteroids.size(); i++) {
                float dist = glm::length(asteroids[i].position - shipPos);
                if (dist < SHIP_RADIUS + asteroids[i].scale)
                    hitShip[i] = true;
            }

            bool anyHit = false;
            for (int i = 0; i < (int)asteroids.size(); i++) {
                if (!hitShip[i]) continue;
                anyHit = true;

                // Explosion centred on the ship
                int shipCount = 50;
                for (int k = 0; k < shipCount && (int)particles.size() < MAX_PT; k++) {
                    Particle p;
                    p.pos     = shipPos + randUnitVec() * randF(0.2f, 1.5f);
                    p.vel     = randUnitVec() * randF(8.f, 28.f);
                    p.maxLife = randF(0.7f, 1.8f);
                    p.life    = p.maxLife;
                    p.size    = randF(12.f, 24.f);
                    particles.push_back(p);
                }

                // Explosion centred on the asteroid
                glm::vec3 astCenter = asteroids[i].position;
                int astCount = 15 + (int)(asteroids[i].scale * 5.f);
                for (int k = 0; k < astCount && (int)particles.size() < MAX_PT; k++) {
                    Particle p;
                    p.pos     = astCenter + randUnitVec() * asteroids[i].scale * 0.4f;
                    p.vel     = randUnitVec() * randF(4.f, 18.f);
                    p.maxLife = randF(0.5f, 1.2f);
                    p.life    = p.maxLife;
                    p.size    = randF(8.f, 18.f);
                    particles.push_back(p);
                }
            }

            if (anyHit) {
                shipInvincibleUntil = currentFrame + SHIP_INVINCIBILITY;
                std::vector<AsteroidInstance> survivors;
                survivors.reserve(asteroids.size());
                for (int i = 0; i < (int)asteroids.size(); i++) {
                    if (!hitShip[i]) survivors.push_back(asteroids[i]);
                }
                asteroids = std::move(survivors);
            }
        }

        // Clear
        glClearColor(0.01f, 0.01f, 0.02f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Viewport aspect ratio
        int vpW, vpH;
        glfwGetFramebufferSize(window, &vpW, &vpH);
        float aspect = vpH > 0 ? (float)vpW / (float)vpH : 1.f;

        // Always look at the ship so it stays centered on screen
        glm::mat4 view       = glm::lookAt(camera.position,
                                           shipPos + glm::vec3(0.f, 0.5f, 0.f),
                                           glm::vec3(0.f, 1.f, 0.f));
        glm::mat4 projection = camera.projMatrix(aspect);

        // Draw spaceship (blink during invincibility frames)
        bool drawShip = (currentFrame > shipInvincibleUntil) ||
                        (fmod(currentFrame * 8.f, 1.f) < 0.5f);

        modelShader.use();
        modelShader.setBool("useTriplanar", false);

        glm::mat4 model = glm::mat4(1.f);
        model = glm::translate(model, shipPos);
        model = glm::rotate(model, glm::radians(-camera.yaw),   glm::vec3(0.f, 1.f, 0.f));
        model = glm::rotate(model, glm::radians(-90.f),         glm::vec3(0.f, 1.f, 0.f));  // fix model orientation
        model = glm::rotate(model, glm::radians(camera.pitch),  glm::vec3(1.f, 0.f, 0.f));  // pitch from mouse
        model = glm::scale(model, glm::vec3(0.75f));

        modelShader.setMat4("model",      model);
        modelShader.setMat4("view",       view);
        modelShader.setMat4("projection", projection);
        modelShader.setVec3("viewPos",    camera.position);

        if (drawShip) {
            for (auto& mesh : spaceship.meshes) {
                modelShader.setBool ("hasDiffuse", mesh.material.hasDiffuse);
                modelShader.setVec3 ("matKd",      mesh.material.Kd);
                if (mesh.material.hasDiffuse) {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, mesh.material.diffuseTexture);
                    modelShader.setInt("diffuseMap", 0);
                }
                mesh.draw();
            }
        }

        // Draw asteroids
        modelShader.setBool("useTriplanar", true);
        for (auto& inst : asteroids) {
            glm::mat4 am = glm::mat4(1.f);
            am = glm::translate(am, inst.position);
            am = glm::rotate(am, glm::radians(inst.rotAngle + inst.rotSpeed * currentFrame),
                             inst.rotAxis);
            am = glm::scale(am, glm::vec3(inst.scale));

            Mesh& amesh = asteroidPool[inst.meshIdx].mesh;
            modelShader.setMat4("model",     am);
            modelShader.setBool("hasDiffuse", amesh.material.hasDiffuse);
            modelShader.setVec3("matKd",      amesh.material.Kd);
            if (amesh.material.hasDiffuse) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, amesh.material.diffuseTexture);
            }
            amesh.draw();
        }

        // Draw skybox
        skyboxShader.use();
        // Strip translation from view so the skybox never moves
        glm::mat4 skyView = glm::mat4(glm::mat3(view));
        skyboxShader.setMat4("view",       skyView);
        skyboxShader.setMat4("projection", projection);
        skyboxShader.setInt ("skybox", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.cubemapTexture);
        skybox.draw();

        // Draw fire trail particles
        if (!particles.empty()) {
            std::vector<float> gpuBuf;
            gpuBuf.reserve(particles.size() * 8);
            for (auto& p : particles) {
                float t = p.life / p.maxLife; // 1=fresh, 0=dead
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

            particleShader.use();
            particleShader.setMat4("view",       view);
            particleShader.setMat4("projection", projection);
            particleShader.setBool("isPoint",    true);

            glDepthMask(GL_FALSE);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE); // additive for glow
            glBindVertexArray(ptVAO);
            glDrawArrays(GL_POINTS, 0, (GLsizei)particles.size());
            glBindVertexArray(0);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_TRUE);
        }

        // Draw asteroid blue trail particles
        if (!asteroidParticles.empty()) {
            std::vector<float> aptBuf;
            aptBuf.reserve(asteroidParticles.size() * 8);
            for (auto& p : asteroidParticles) {
                float t = p.life / p.maxLife; // 1=fresh, 0=dead
                glm::vec3 col;
                float alpha;
                if (t > 0.5f) {
                    float u = (t - 0.5f) * 2.f;
                    col   = glm::mix(glm::vec3(0.2f, 0.5f, 1.0f), glm::vec3(0.6f, 0.9f, 1.0f), u);
                    alpha = glm::mix(0.7f, 0.9f, u);
                } else {
                    float u = t * 2.f;
                    col   = glm::mix(glm::vec3(0.0f, 0.05f, 0.3f), glm::vec3(0.2f, 0.5f, 1.0f), u);
                    alpha = u * 0.7f;
                }
                float sz = p.size * t;
                aptBuf.push_back(p.pos.x); aptBuf.push_back(p.pos.y); aptBuf.push_back(p.pos.z);
                aptBuf.push_back(col.r);   aptBuf.push_back(col.g);   aptBuf.push_back(col.b);
                aptBuf.push_back(alpha);
                aptBuf.push_back(sz);
            }
            glBindBuffer(GL_ARRAY_BUFFER, aptVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0,
                            (GLsizeiptr)(aptBuf.size() * sizeof(float)), aptBuf.data());

            particleShader.use();
            particleShader.setMat4("view",       view);
            particleShader.setMat4("projection", projection);
            particleShader.setBool("isPoint",    true);

            glDepthMask(GL_FALSE);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glBindVertexArray(aptVAO);
            glDrawArrays(GL_POINTS, 0, (GLsizei)asteroidParticles.size());
            glBindVertexArray(0);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_TRUE);
        }

        // Draw stars (billboards at "infinity")
        glDepthFunc(GL_LEQUAL);
        starShader.use();
        starShader.setMat4("view",       view);
        starShader.setMat4("projection", projection);
        glBindVertexArray(sunVAO);
        for (auto& vs : visualStars) {
            starShader.setVec3 ("sunDir",    vs.dir);
            starShader.setFloat("sunRadius", vs.radius);
            starShader.setVec3 ("starColor", vs.color);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);

        glfwSwapBuffers(window);
    }

    // Cleanup
    glDeleteVertexArrays(1, &ptVAO);
    glDeleteBuffers(1, &ptVBO);
    glDeleteVertexArrays(1, &aptVAO);
    glDeleteBuffers(1, &aptVBO);
    glDeleteVertexArrays(1, &sunVAO);
    glDeleteBuffers(1, &sunVBO);
    glDeleteBuffers(1, &sunEBO);
    for (auto& a : asteroidPool) a.free();
    spaceship.free();
    skybox.free();
    glfwTerminate();
    return 0;
}