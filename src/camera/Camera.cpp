//
// Created by Dmitri Wamback on 2026-05-30.
//

#include "Camera.h"

#include "../VulkanResources.h"

void Camera::Initialize() {

    yaw = -glm::pi<float>()/2.0f;

    position = glm::vec3(0.0f, 0.0f, 3.0f);
    lookDirection = glm::vec3(0.0f, 0.0f, -1.0f);

    projection = glm::perspective(glm::radians(60.0f), 1200.0f / 800.0f, 0.1f, 1000.0f);
    view = glm::lookAt(position, position + lookDirection, glm::vec3(0.0f, 1.0f, 0.0f));
}

void Camera::Update(glm::vec4 movement, float up, float down, VulkanResources& resources) {

    int width, height;
    glfwGetFramebufferSize(resources.window, &width, &height);

    float aspect = (float)width / (float)height;

    projection = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 1000.0f);

    float forward = movement.x,
          backward = movement.y,
          left = movement.z,
          right = movement.w;

    glm::vec3 motion = lookDirection;

    glm::vec3 rightMotion = glm::normalize(glm::cross(motion, glm::vec3(0.0f, 1.0f, 0.0f)));

    float forwardMotion = forward + backward,
          sidewaysMotion = -(left + right),
          verticalMotion = up + down;

    glm::vec3 straightVelocity = motion * forwardMotion * speed;
    glm::vec3 rightVelocity = rightMotion * sidewaysMotion * speed;
    glm::vec3 verticalVelocity = glm::vec3(0.0f, 1.0f, 0.0f) * verticalMotion * speed;

    position += straightVelocity + rightVelocity + verticalVelocity;

    lookDirection = glm::normalize(glm::vec3(cos(yaw) * cos(pitch),
                                               sin(pitch),
                                               sin(yaw) * cos(pitch)));

    view = glm::lookAt(position, position + lookDirection, glm::vec3(0.0f, 1.0f, 0.0f));
}

Camera::~Camera() {

}
