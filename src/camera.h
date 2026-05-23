#pragma once
#include <glm/glm.hpp>

class Camera
{
public:
    glm::vec3 position  = {0.f, 2.f, 10.f};
    glm::vec3 front     = {0.f, 0.f, -1.f};

    float yaw   = -90.f;
    float pitch =   0.f;
    float sensitivity = 0.1f;
    float fov   = 45.f;

    glm::mat4 projMatrix(float aspect, float nearZ = 0.1f, float farZ = 1000.f) const;

    void processMouse(float xOffset, float yOffset);

private:
    void updateVectors();
};