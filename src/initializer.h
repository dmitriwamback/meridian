//
// Created by Dmitri Wamback on 2026-05-30.
//

#ifndef MERIDIAN_INITIALIZER_H
#define MERIDIAN_INITIALIZER_H

#include <iostream>
#include <cstring>
#include <set>
#include <optional>
#include <vector>
#include <string>

#include <GLFW/glfw3.h>

#include "core/buffer/Vertex.h"
#include "memory/Internal.h"

#include "VulkanResources.h"

#define ENABLE_VALIDATION_LAYERS true

#if defined(__APPLE__)

#else

#endif

const std::vector<const char*> VALIDATION_LAYERS = {
    "VK_LAYER_KHRONOS_validation"
};

static bool CheckValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : VALIDATION_LAYERS) {
        bool found = false;

        for (const auto& layer : availableLayers) {
            if (strcmp(layerName, layer.layerName) == 0) {
                found = true;
                break;
            }
        }

        if (!found) return false;
    }

    return true;
}

void Initialize(GLFWwindow* window, VulkanResources& resources) {

    resources.window = window;

    resources.instance = Meridian::Internal::CreateInstance(
        "Meridian",
        "No Engine",
        VK_MAKE_VERSION(1, 0, 0),
        VK_MAKE_VERSION(1, 0, 0),
        VK_API_VERSION_1_2,
        resources
    );

    if (glfwCreateWindowSurface(resources.instance, window, nullptr, &resources.surface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create window surface!");
    }

    resources.physicalDevice = Meridian::Internal::PickPhysicalDevice(
        resources.instance,
        resources.surface
    );

    Meridian::QueueFamilyIndices indices = Meridian::Internal::FindQueueFamily(
        resources.physicalDevice,
        resources.surface
    );
    resources.graphicsQueueFamily = indices.graphicsFamily.value();
    resources.presentQueueFamily = indices.presentQueue.value();

    Meridian::Internal::CreateDevice(
        resources.physicalDevice,
        resources.graphicsQueueFamily,
        resources.presentQueueFamily,
        resources.device,
        resources.graphicsQueue,
        resources.presentQueue
    );

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    VkExtent2D windowExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
    resources.windowExtent = windowExtent;

    Meridian::Internal::CreateSwapchain(
        resources.device,
        resources.physicalDevice,
        resources.surface,
        resources.graphicsQueueFamily,
        resources.presentQueueFamily,
        resources.windowExtent,
        resources.swapchain,
        resources.swapchainImages,
        resources.swapchainImageViews,
        resources.swapchainImageFormat,
        resources.swapchainExtent
    );

    resources.depthFormat = Meridian::Internal::FindDepthFormat(resources.physicalDevice);

    Meridian::Internal::CreateDepthImage(
        resources.device,
        resources.physicalDevice,
        resources.swapchainExtent,
        resources.depthFormat,
        resources.depthImage,
        resources.depthImageMemory,
        resources.depthImageView
    );

    Meridian::Internal::CreateRenderPass(
        resources.device,
        resources.swapchainImageFormat,
        resources.depthFormat,
        resources.renderPass
    );

    Meridian::Internal::CreateFramebuffers(
        resources.device,
        resources.renderPass,
        resources.swapchainImageViews,
        resources.swapchainExtent,
        resources.framebuffers,
        resources.depthImageView
    );

    Meridian::Internal::CreateCommandPool(
        resources.device,
        resources.graphicsQueueFamily,
        resources.commandPool
    );

    resources.commandBuffers.resize(IN_FLIGHT_FRAMES);
    Meridian::Internal::CreateCommandBuffers(
        resources.device,
        resources.commandPool,
        resources.commandBuffers,
        IN_FLIGHT_FRAMES
    );

    Meridian::Internal::CreateSyncObjects(
        resources.device,
        resources.imageAvailableSemaphores,
        resources.renderCompleteSemaphores,
        resources.inFlightFences
    );

    Meridian::Internal::CreateDescriptorSetLayout(resources.device, resources.descriptorSetLayout);

    Meridian::Internal::CreateDescriptorPool(resources.device, IN_FLIGHT_FRAMES, resources.descriptorPool);

    Meridian::Internal::CreateDescriptorSets(
        resources.device,
        resources.descriptorPool,
        resources.descriptorSetLayout,
        IN_FLIGHT_FRAMES,
        resources.descriptorSets);

    std::cout << "Vulkan initialized successfully!" << std::endl;
}

void Cleanup(VulkanResources& resources) {

    vkDeviceWaitIdle(resources.device);

    if (resources.depthImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(resources.device, resources.depthImageView, nullptr);
    }
    if (resources.depthImage != VK_NULL_HANDLE) {
        vkDestroyImage(resources.device, resources.depthImage, nullptr);
    }
    if (resources.depthImageMemory != VK_NULL_HANDLE) {
        vkFreeMemory(resources.device, resources.depthImageMemory, nullptr);
    }

    for (auto framebuffer : resources.framebuffers) {
        vkDestroyFramebuffer(resources.device, framebuffer, nullptr);
    }

    vkDestroyRenderPass(resources.device, resources.renderPass, nullptr);

    for (size_t i = 0; i < IN_FLIGHT_FRAMES; i++) {
        vkDestroySemaphore(resources.device, resources.renderCompleteSemaphores[i], nullptr);
        vkDestroySemaphore(resources.device, resources.imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(resources.device, resources.inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(resources.device, resources.commandPool, nullptr);

    for (auto imageView : resources.swapchainImageViews) {
        vkDestroyImageView(resources.device, imageView, nullptr);
    }
    vkDestroySwapchainKHR(resources.device, resources.swapchain, nullptr);

    vkDestroyDevice(resources.device, nullptr);
    vkDestroySurfaceKHR(resources.instance, resources.surface, nullptr);
    vkDestroyInstance(resources.instance, nullptr);

    std::cout << "Vulkan cleaned up successfully!" << std::endl;
}

void RecreateSwapchain(GLFWwindow* window, VulkanResources& resources) {
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(resources.device);

    for (auto imageView : resources.swapchainImageViews) {
        vkDestroyImageView(resources.device, imageView, nullptr);
    }
    vkDestroySwapchainKHR(resources.device, resources.swapchain, nullptr);
    vkDestroyRenderPass(resources.device, resources.renderPass, nullptr);
    for (auto framebuffer : resources.framebuffers) {
        vkDestroyFramebuffer(resources.device, framebuffer, nullptr);
    }
    resources.framebuffers.clear();
    resources.swapchainImageViews.clear();
    resources.swapchainImages.clear();

    VkExtent2D windowExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

    Meridian::Internal::CreateSwapchain(
        resources.device,
        resources.physicalDevice,
        resources.surface,
        resources.graphicsQueueFamily,
        resources.presentQueueFamily,
        windowExtent,
        resources.swapchain,
        resources.swapchainImages,
        resources.swapchainImageViews,
        resources.swapchainImageFormat,
        resources.swapchainExtent
    );

    Meridian::Internal::CreateRenderPass(
        resources.device,
        resources.swapchainImageFormat,
        resources.depthFormat,
        resources.renderPass
    );

    Meridian::Internal::CreateFramebuffers(
        resources.device,
        resources.renderPass,
        resources.swapchainImageViews,
        resources.swapchainExtent,
        resources.framebuffers,
        resources.depthImageView
    );
}

#endif //MERIDIAN_INITIALIZER_H