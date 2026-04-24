#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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

// Globals
static Camera camera;
static float  lastX      = SCR_W / 2.f;
static float  lastY      = SCR_H / 2.f;
static bool   firstMouse = true;
static float  deltaTime  = 0.f;
static float  lastFrame  = 0.f;

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

// Continuous key handling
static void processInput(GLFWwindow* win)
{
    const int keys[] = {
        GLFW_KEY_W, GLFW_KEY_S, GLFW_KEY_A, GLFW_KEY_D,
        GLFW_KEY_SPACE, GLFW_KEY_LEFT_SHIFT
    };
    for (int k : keys)
        if (glfwGetKey(win, k) == GLFW_PRESS)
            camera.processKeyboard(k, deltaTime);
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

    // Shaders
    Shader modelShader ("shaders/model.vert",  "shaders/model.frag");
    Shader skyboxShader("shaders/skybox.vert", "shaders/skybox.frag");

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

    // Asteroids – each has a different seed → unique shape
    struct AsteroidInstance {
        Asteroid   asteroid;
        glm::vec3  position;
        float      scale;
        float      rotSpeed; // degrees per second
    };

    std::vector<AsteroidInstance> asteroids;
    struct AsteroidDef { int seed; glm::vec3 pos; float scale; float rotSpeed; };
    for (auto& d : std::initializer_list<AsteroidDef>{
        { 0, {  12.f,  2.f, -18.f }, 2.5f,  8.f },
        { 1, { -10.f, -3.f, -22.f }, 1.8f, 12.f },
        { 2, {   5.f,  5.f, -30.f }, 3.2f,  5.f },
        { 3, { -18.f,  1.f, -15.f }, 1.2f, 20.f },
        { 4, {  20.f, -4.f, -25.f }, 2.0f,  7.f },
    }) {
        AsteroidInstance inst;
        inst.asteroid.generate(d.seed);
        inst.position  = d.pos;
        inst.scale     = d.scale;
        inst.rotSpeed  = d.rotSpeed;
        asteroids.push_back(std::move(inst));
    }

    GLuint asteroidTex = loadTexture("assets/textures/asteroid/photo-stone-texture-pattern.jpg");
    for (auto& inst : asteroids) {
        inst.asteroid.mesh.material.diffuseTexture = asteroidTex;
        inst.asteroid.mesh.material.hasDiffuse     = true;
    }

    // Pre-configure skybox shader sampler
    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    // Pre-configure model shader
    modelShader.use();
    modelShader.setInt  ("diffuseMap",    0);
    modelShader.setVec3 ("lightDir",      glm::normalize(glm::vec3(-0.3f, -1.f, -0.5f)));
    modelShader.setVec3 ("lightColor",    {1.f,  1.f,  1.f });
    modelShader.setVec3 ("ambientColor",  {0.15f,0.15f,0.15f});

    // Game loop
    while (!glfwWindowShouldClose(window))
    {
        // Timing
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Input
        glfwPollEvents();
        processInput(window);

        // Clear
        glClearColor(0.01f, 0.01f, 0.02f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Viewport aspect ratio
        int vpW, vpH;
        glfwGetFramebufferSize(window, &vpW, &vpH);
        float aspect = vpH > 0 ? (float)vpW / (float)vpH : 1.f;

        glm::mat4 view       = camera.viewMatrix();
        glm::mat4 projection = camera.projMatrix(aspect);

        // Draw spaceship
        modelShader.use();

        glm::mat4 model = glm::mat4(1.f);
        // scale Blender OBJ
        model = glm::scale(model, glm::vec3(0.5f));
        // Rotate so +Z faces "forward" in our scene
        model = glm::rotate(model, glm::radians(-90.f), glm::vec3(0,1,0));

        modelShader.setMat4("model",      model);
        modelShader.setMat4("view",       view);
        modelShader.setMat4("projection", projection);
        modelShader.setVec3("viewPos",    camera.position);

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

        // Draw asteroids
        for (auto& inst : asteroids) {
            glm::mat4 am = glm::mat4(1.f);
            am = glm::translate(am, inst.position);
            am = glm::rotate(am, glm::radians(inst.rotSpeed * (float)glfwGetTime()),
                             glm::vec3(0.3f, 1.f, 0.2f));
            am = glm::scale(am, glm::vec3(inst.scale));

            modelShader.setMat4("model", am);
            modelShader.setBool("hasDiffuse", inst.asteroid.mesh.material.hasDiffuse);
            modelShader.setVec3("matKd",      inst.asteroid.mesh.material.Kd);
            if (inst.asteroid.mesh.material.hasDiffuse) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, inst.asteroid.mesh.material.diffuseTexture);
            }
            inst.asteroid.mesh.draw();
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

        glfwSwapBuffers(window);
    }

    // Cleanup
    spaceship.free();
    skybox.free();
    glfwTerminate();
    return 0;
}