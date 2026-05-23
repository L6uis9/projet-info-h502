#include "camera.h"
#include <glm/gtc/matrix_transform.hpp>

glm::mat4 Camera::projMatrix(float aspect, float nearZ, float farZ) const
{
    return glm::perspective(glm::radians(fov), aspect, nearZ, farZ);
}

void Camera::updateVectors()
{
    glm::vec3 f;
    f.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    f.y = sin(glm::radians(pitch));
    f.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(f);
}

void Camera::processMouse(float xOffset, float yOffset)
{
    yaw   += xOffset * sensitivity;
    pitch += yOffset * sensitivity;
    if (pitch >  89.f) pitch =  89.f;
    if (pitch < -89.f) pitch = -89.f;
    updateVectors();
}
