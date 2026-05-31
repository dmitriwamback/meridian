//
// Created by Dmitri Wamback on 2026-05-30.
//

#ifndef MERIDIAN_CAMERA_H
#define MERIDIAN_CAMERA_H

#include <GLFW/glfw3.h>
#include <glm/glm/glm.hpp>
#include <glm/glm/vec3.hpp>
#include <glm/glm/mat4x4.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>

#include "../VulkanResources.h"

class Camera {
public:

    Camera() = default;
    ~Camera();

    glm::mat4 projection, view;
    glm::vec3 position, lookDirection;

    float pitch, yaw;
    float speed = 1.0f;

    float lastMouseX, lastMouseY;
    int mouseButton = GLFW_MOUSE_BUTTON_RIGHT;

    void Initialize();
    void Update(glm::vec4 movement, float up, float down, VulkanResources& resources);
};


#endif //MERIDIAN_CAMERA_H