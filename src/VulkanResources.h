//
// Created by Dmitri Wamback on 2026-05-30.
//

#ifndef MERIDIAN_VULKANRESOURCES_H
#define MERIDIAN_VULKANRESOURCES_H

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>

struct VulkanResources {
    GLFWwindow* window;

    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    uint32_t graphicsQueueFamily = 0;
    uint32_t presentQueueFamily = 0;

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    VkFormat swapchainImageFormat;
    VkExtent2D swapchainExtent;

    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;

    std::vector<VkFramebuffer> framebuffers;
    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderCompleteSemaphores;
    std::vector<VkFence> inFlightFences;

    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptorSets;

    VkExtent2D windowExtent;

    uint32_t currentFrame = 0;
};

#endif //MERIDIAN_VULKANRESOURCES_H