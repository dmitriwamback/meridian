//
// Created by Dmitri Wamback on 2026-05-30.
//

#ifndef MERIDIAN_RENDERER_H
#define MERIDIAN_RENDERER_H

#include "../pipeline/PipelineManager.h"
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "../VulkanResources.h"
#include "../core/buffer/UniformBuffer.h"
#include <vector>
#include <string>

class Renderer {
public:
    Renderer() = default;
    ~Renderer();

    void Initialize(GLFWwindow* window, VulkanResources& vulkan);

    void Render(VulkanResources& vulkan);
    void RecreateSwapchain(VulkanResources& vulkan);
    void Cleanup(VulkanResources& vulkan);

    PipelineManager& GetPipelineManager();
    const PipelineManager& GetPipelineManager() const;

    bool IsSwapchainOutOfRange() const;

private:
    void CreatePipelineLayouts(VulkanResources& vulkan);

    PipelineManager pipelineManager;
    VkPipelineLayout standardPipelineLayout = VK_NULL_HANDLE;
    VkPipelineLayout gbufferPipelineLayout = VK_NULL_HANDLE;
    VkPipelineLayout debugPipelineLayout = VK_NULL_HANDLE;

    std::unique_ptr<UniformBuffer> uniformBuffer;
    UniformBufferObject uniformBufferObject{};

    bool swapchainOutOfDate = false;
    bool initialized = false;
};

#endif // MERIDIAN_RENDERER_H