#include <iostream>
#include <vulkan/vulkan.h>
#include <chrono>
#include <iomanip>

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

    glfwSetWindowUserPointer(window, &renderer);
    glfwSetCursorPosCallback(window, Renderer::CursorPosCallback);

    auto frameStartTime = std::chrono::high_resolution_clock::now();
    int frameCount = 0;
    double lastFpsUpdateTime = 0.0;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        if (renderer.IsSwapchainOutOfRange()) {
            RecreateSwapchain(window, vulkan);
        }

        renderer.Render(vulkan);

        glfwSwapBuffers(window);

        frameCount++;
        double currentTime = glfwGetTime();

        if (currentTime - lastFpsUpdateTime >= 1.0) {
            double fps = frameCount / (currentTime - lastFpsUpdateTime);

            std::cout << "\rFPS: " << std::fixed << std::setprecision(2) << fps
                      << " | Frames: " << frameCount
                      << std::flush;

            frameCount = 0;
            lastFpsUpdateTime = currentTime;
        }
    }

    // Print final FPS
    std::cout << std::endl;

    vkDeviceWaitIdle(vulkan.device);
    renderer.Cleanup(vulkan);
    Cleanup(vulkan);

    return 0;
}