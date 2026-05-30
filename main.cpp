#include <iostream>
#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "src/initializer.h"
#include "src/scene/Renderer.h"

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(1200, 800, "Meridian", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    VulkanResources vulkan;
    Initialize(window, vulkan);

    Renderer renderer;
    renderer.Initialize(window, vulkan);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        if (renderer.IsSwapchainOutOfRange()) {
            RecreateSwapchain(window, vulkan);
        }

        renderer.Render(vulkan);

        glfwSwapBuffers(window);
    }

    vkDeviceWaitIdle(vulkan.device);
    renderer.Cleanup(vulkan);
    Cleanup(vulkan);

    return 0;
}