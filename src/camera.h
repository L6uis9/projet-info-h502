#pragma once
#include <glm/glm.hpp>

class Camera
{
public:
    glm::vec3 position  = {0.f, 2.f, 10.f};
    glm::vec3 front     = {0.f, 0.f, -1.f};

    float yaw   = -90.f;  // -90° faces -Z (OpenGL's default forward)
    float pitch =   0.f;
    float sensitivity = 0.1f;
    float fov   = 45.f;

    // Returns a perspective projection matrix for the given aspect ratio.
    glm::mat4 projMatrix(float aspect, float nearZ = 0.1f, float farZ = 1000.f) const;

    // Applies mouse delta to yaw/pitch and recomputes the front vector.
    void processMouse(float xOffset, float yOffset);

private:
    // Recomputes front from the current yaw and pitch angles.
    void updateVectors();
};