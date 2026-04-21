#include "camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

glm::mat4 Camera::viewMatrix() const
{
    return glm::lookAt(position, position + front, up);
}

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
    // right & up auto-derive from world up
    glm::vec3 right = glm::normalize(glm::cross(front, {0,1,0}));
    up = glm::normalize(glm::cross(right, front));
}

void Camera::processKeyboard(int key, float dt)
{
    float v = speed * dt;
    if (key == GLFW_KEY_W || key == GLFW_KEY_Z) position += v * front;
    if (key == GLFW_KEY_S) position -= v * front;
    if (key == GLFW_KEY_A || key == GLFW_KEY_Q) position -= glm::normalize(glm::cross(front, up)) * v;
    if (key == GLFW_KEY_D) position += glm::normalize(glm::cross(front, up)) * v;
    if (key == GLFW_KEY_SPACE)      position.y += v;
    if (key == GLFW_KEY_LEFT_SHIFT) position.y -= v;
}

void Camera::processMouse(float xOffset, float yOffset)
{
    yaw   += xOffset * sensitivity;
    pitch += yOffset * sensitivity;
    if (pitch >  89.f) pitch =  89.f;
    if (pitch < -89.f) pitch = -89.f;
    updateVectors();
}