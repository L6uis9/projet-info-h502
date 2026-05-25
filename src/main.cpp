#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

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
#include "particle.h"
#include "textureLoader.h"
#include "utils.h"
#include "postprocess.h"
#include "star.h"

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
static const float ASTEROID_MIN_SPEED      = 5.f;
static const float ASTEROID_MAX_SPEED      = 20.f;
static const float ASTEROID_SPAWN_INTERVAL = 0.01f;  // seconds between spawns
static const int   ASTEROID_MAX_COUNT      = 200;
static const float ASTEROID_TOWARD_SHIP_PROBA = 0.2f;

// Ship collision settings
static const float SHIP_RADIUS        = 1.5f;
static const float SHIP_INVINCIBILITY = 2.0f;

// Ship physics settings
static const float SHIP_ACCEL = 50.f;
static const float SHIP_DRAG  = 1.f;

// Camera follow settings
static const float CAM_FOLLOW_DIST   = 8.f;
static const float CAM_FOLLOW_HEIGHT = 1.5f;

// Motion blur settings
static const float BLUR_MAX_STRENGTH = 0.08f;

// Ship hit burst particle settings
static const int   HIT_BURST_COUNT   = 50;
static const float HIT_SPAWN_RADIUS_MIN = 0.2f;
static const float HIT_SPAWN_RADIUS_MAX = 1.5f;
static const float HIT_SPEED_MIN     = 8.f;
static const float HIT_SPEED_MAX     = 28.f;
static const float HIT_LIFE_MIN      = 0.7f;
static const float HIT_LIFE_MAX      = 1.8f;
static const float HIT_SIZE_MIN      = 12.f;
static const float HIT_SIZE_MAX      = 24.f;

// Fire trail particle settings
static const float TRAIL_BACK_OFFSET  = 1.2f;
static const float TRAIL_SPREAD_RIGHT = 0.45f;
static const float TRAIL_SPREAD_UP    = 0.5f;
static const float TRAIL_SPEED_MIN    = 8.f;
static const float TRAIL_SPEED_MAX    = 16.f;
static const float TRAIL_SIDE_SPREAD  = 1.5f;
static const float TRAIL_VERT_SPREAD  = 1.f;
static const float TRAIL_LIFE_MIN     = 0.2f;
static const float TRAIL_LIFE_MAX     = 0.35f;
static const float TRAIL_SIZE_MIN     = 4.f;
static const float TRAIL_SIZE_MAX     = 8.f;

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
static bool       shipReflective = false;

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
    if (key == GLFW_KEY_X && action == GLFW_PRESS)
        shipReflective = !shipReflective;
}

// Continuous key handling – updates ship velocity, then snaps camera behind it
static void processInput(GLFWwindow* win)
{
    glm::vec3 fwd = camera.front;

    if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(win, GLFW_KEY_Z) == GLFW_PRESS)
        shipVelocity += fwd * SHIP_ACCEL * deltaTime;

    shipVelocity *= std::max(0.f, 1.f - SHIP_DRAG * deltaTime);

    lookBack = (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS);

    float side  = lookBack ? 1.f : -1.f;
    camera.position = shipPos + fwd * CAM_FOLLOW_DIST * side + glm::vec3(0.f, CAM_FOLLOW_HEIGHT, 0.f);
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
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Init GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        throw std::runtime_error("gladLoadGLLoader failed");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Shaders
    Shader modelShader    ("shaders/model.vert",      "shaders/model.frag");
    Shader skyboxShader   ("shaders/skybox.vert",     "shaders/skybox.frag");
    Shader starShader     ("shaders/star.vert",       "shaders/star.frag");
    Shader particleShader ("shaders/particle.vert",   "shaders/particle.frag");
    Shader motionBlurShader("shaders/motionblur.vert","shaders/motionblur.frag");

    // Skybox
    // px=right, nx=left, py=top, ny=bottom, pz=front, nz=back
    Skybox skybox;
    skybox.load({
        "assets/cubemap/px.png", "assets/cubemap/nx.png",
        "assets/cubemap/py.png", "assets/cubemap/ny.png",
        "assets/cubemap/pz.png", "assets/cubemap/nz.png"
    });

    // Spaceship model
    Model spaceship;
    spaceship.load("assets/models/spaceship.obj", "assets/textures/spaceship");

    // Earth planet
    Model earth;
    earth.load("assets/models/Earth 2K.obj", "assets/textures/earth");
    static const glm::vec3 EARTH_POS   = {0.f, -50.f, -700.f};
    static const float     EARTH_SCALE = 50.f;

    // Asteroid system
    srand(42);
    AsteroidSpawnCfg astCfg {
        ASTEROID_MESH_POOL,
        ASTEROID_MIN_SPAWN_RADIUS,
        ASTEROID_SPAWN_RADIUS,
        ASTEROID_DESPAWN_DIST,
        ASTEROID_MIN_SCALE,
        ASTEROID_MAX_SCALE,
        ASTEROID_MIN_SPEED,
        ASTEROID_MAX_SPEED,
        ASTEROID_SPAWN_INTERVAL,
        ASTEROID_MAX_COUNT,
        ASTEROID_TOWARD_SHIP_PROBA,
    };

    GLuint asteroidTex = loadTexture("assets/textures/asteroid/photo-stone-texture-pattern.jpg");
    AsteroidSystem asteroids;
    asteroids.init(astCfg, asteroidTex);

    // Particle system
    ParticleSystem particles;
    particles.init();

    // Pre-configure skybox shader sampler
    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    const std::vector<StarEntry> visualStars = {
        { glm::normalize(glm::vec3(4000.f, 1500.f, -2000.f)), glm::vec3(1.00f, 0.80f, 0.50f), 0.18f },
    };

    // Pre-configure model shader
    modelShader.use();
    modelShader.setInt ("diffuseMap",   0);
    modelShader.setInt ("skyboxMap",    1);
    modelShader.setVec3("ambientColor", {0.005f, 0.005f, 0.008f});
    modelShader.setInt ("numStars",     (int)visualStars.size());
    for (int i = 0; i < (int)visualStars.size(); i++) {
        modelShader.setVec3("starDirs["   + std::to_string(i) + "]", visualStars[i].dir);
        modelShader.setVec3("starColors[" + std::to_string(i) + "]", visualStars[i].color);
    }

    Star sunBillboard;
    sunBillboard.init();

    int fboW, fboH;
    glfwGetFramebufferSize(window, &fboW, &fboH);
    PostProcess postProcess;
    postProcess.init(fboW, fboH);

    // Game loop
    while (!glfwWindowShouldClose(window))
    {
        // Timing
        float currentFrame = (float)glfwGetTime();
        deltaTime  = currentFrame - lastFrame;
        lastFrame  = currentFrame;

        // Input
        glfwPollEvents();
        shipPos += shipVelocity * deltaTime;
        processInput(window);

        // Debug title
        {
            char title[128];
            snprintf(title, sizeof(title),
                "Space Game | spd: %.1f  pos: (%.1f, %.1f, %.1f)",
                glm::length(shipVelocity), shipPos.x, shipPos.y, shipPos.z);
            glfwSetWindowTitle(window, title);
        }

        // Fire trail: spawn particles when thrusting
        bool thrusting = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS ||
                         glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS;
        if (thrusting) {
            particles.spawnAccum += ParticleSystem::SPAWN_RATE * deltaTime;
            int toSpawn = (int)particles.spawnAccum;
            particles.spawnAccum -= (float)toSpawn;

            glm::vec3 back  = -camera.front;
            glm::vec3 right = glm::normalize(glm::cross(camera.front, glm::vec3(0.f, 1.f, 0.f)));
            glm::vec3 up    = glm::cross(right, camera.front);

            for (int i = 0; i < toSpawn && !particles.full(); ++i)
                particles.add(
                    shipPos + back * TRAIL_BACK_OFFSET + right * randF(-TRAIL_SPREAD_RIGHT, TRAIL_SPREAD_RIGHT) + up * randF(-TRAIL_SPREAD_UP, TRAIL_SPREAD_UP),
                    back * randF(TRAIL_SPEED_MIN, TRAIL_SPEED_MAX) + right * randF(-TRAIL_SIDE_SPREAD, TRAIL_SIDE_SPREAD) + up * randF(-TRAIL_VERT_SPREAD, TRAIL_VERT_SPREAD),
                    randF(TRAIL_LIFE_MIN, TRAIL_LIFE_MAX), randF(TRAIL_SIZE_MIN, TRAIL_SIZE_MAX));
        }

        particles.update(deltaTime);

        // Asteroid system update
        asteroids.trySpawn(currentFrame, shipPos);
        for (auto& exp : asteroids.update(deltaTime, shipPos))
            particles.spawnExplosion(exp);

        // Ship collision
        std::vector<AsteroidExplosion> shipHits;
        if (asteroids.checkShipCollision(shipPos, SHIP_RADIUS, currentFrame,
                                          shipInvincibleUntil, SHIP_INVINCIBILITY, shipHits)) {
            for (int k = 0; k < HIT_BURST_COUNT && !particles.full(); k++)
                particles.add(shipPos + randUnitVec() * randF(HIT_SPAWN_RADIUS_MIN, HIT_SPAWN_RADIUS_MAX),
                              randUnitVec() * randF(HIT_SPEED_MIN, HIT_SPEED_MAX),
                              randF(HIT_LIFE_MIN, HIT_LIFE_MAX), randF(HIT_SIZE_MIN, HIT_SIZE_MAX));
            for (auto& exp : shipHits)
                particles.spawnExplosion(exp);
        }

        postProcess.begin();

        // Viewport aspect ratio
        int vpW, vpH;
        glfwGetFramebufferSize(window, &vpW, &vpH);
        float aspect = vpH > 0 ? (float)vpW / (float)vpH : 1.f;

        // Always look at the ship so it stays centered on screen
        glm::mat4 view       = glm::lookAt(camera.position,
                                           shipPos + glm::vec3(0.f, 0.5f, 0.f),
                                           glm::vec3(0.f, 1.f, 0.f));
        glm::mat4 projection = camera.projMatrix(aspect);

        // Draw spaceship (blink during invincibility)
        bool drawShip = (currentFrame > shipInvincibleUntil) ||
                        (fmod(currentFrame * 8.f, 1.f) < 0.5f);

        modelShader.use();
        modelShader.setBool("useTriplanar", false);

        glm::mat4 model = glm::mat4(1.f);
        model = glm::translate(model, shipPos);
        model = glm::rotate(model, glm::radians(-camera.yaw),   glm::vec3(0.f, 1.f, 0.f));
        model = glm::rotate(model, glm::radians(-90.f),         glm::vec3(0.f, 1.f, 0.f));
        model = glm::rotate(model, glm::radians(camera.pitch),  glm::vec3(1.f, 0.f, 0.f));
        model = glm::scale(model, glm::vec3(0.75f));

        modelShader.setMat4("model",      model);
        modelShader.setMat4("view",       view);
        modelShader.setMat4("projection", projection);
        modelShader.setVec3("viewPos",    camera.position);

        if (drawShip) {
            modelShader.setBool("reflective", shipReflective);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.cubemapTexture);
            for (auto& mesh : spaceship.meshes) {
                modelShader.setBool ("hasDiffuse",      mesh.material.hasDiffuse);
                modelShader.setVec3 ("matKd",           mesh.material.Kd);
                modelShader.setBool ("refractive",      mesh.material.refractive);
                modelShader.setFloat("refractiveIndex", mesh.material.Ni);
                if (mesh.material.hasDiffuse && !mesh.material.refractive) {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, mesh.material.diffuseTexture);
                    modelShader.setInt("diffuseMap", 0);
                }
                mesh.draw();
            }
        }
        modelShader.setBool("reflective", false);
        modelShader.setBool("refractive", false);

        // Draw Earth
        modelShader.setBool("useTriplanar", false);
        {
            glm::mat4 em = glm::mat4(1.f);
            em = glm::translate(em, EARTH_POS);
            em = glm::rotate(em, glm::radians(currentFrame * 3.f), glm::vec3(0.f, 1.f, 0.f));
            em = glm::scale(em, glm::vec3(EARTH_SCALE));
            modelShader.setMat4("model", em);

            for (auto& mesh : earth.meshes) {
                if (mesh.material.name != "Earth") continue;
                modelShader.setBool("hasDiffuse", mesh.material.hasDiffuse);
                modelShader.setVec3("matKd",      mesh.material.Kd);
                if (mesh.material.hasDiffuse) {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, mesh.material.diffuseTexture);
                }
                mesh.draw();
            }

            glDepthMask(GL_FALSE);
            glBlendFunc(GL_ONE, GL_ONE);
            for (auto& mesh : earth.meshes) {
                if (mesh.material.name != "Clouds") continue;
                modelShader.setBool("hasDiffuse", mesh.material.hasDiffuse);
                modelShader.setVec3("matKd",      mesh.material.Kd);
                if (mesh.material.hasDiffuse) {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, mesh.material.diffuseTexture);
                }
                mesh.draw();
            }
            glDepthMask(GL_TRUE);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }

        // Draw asteroids
        asteroids.draw(modelShader, currentFrame);

        // Draw skybox
        skyboxShader.use();
        glm::mat4 skyView = glm::mat4(glm::mat3(view));
        skyboxShader.setMat4("view",       skyView);
        skyboxShader.setMat4("projection", projection);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.cubemapTexture);
        skybox.draw();

        // Draw fire trail particles
        particles.draw(view, projection, particleShader);

        // Draw sun at infinity using a billboard (always faces camera)
        sunBillboard.draw(starShader, view, projection, visualStars);

        float blurStrength = glm::clamp(glm::length(shipVelocity) / 100.f, 0.f, 1.f) * BLUR_MAX_STRENGTH;
        postProcess.apply(motionBlurShader, blurStrength);

        glfwSwapBuffers(window);
    }

    // Cleanup
    particles.free();
    asteroids.free();
    postProcess.free();
    sunBillboard.free();
    spaceship.free();
    earth.free();
    skybox.free();
    glfwTerminate();
    return 0;
}
