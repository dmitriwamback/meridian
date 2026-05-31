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

#include "../camera/Camera.h"
#include "../core/objects/Asset.h"

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
    static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);

private:
    void CreatePipelineLayouts(VulkanResources& vulkan);
    void UpdateUniformBuffers(VulkanResources& vulkan);

    PipelineManager pipelineManager;
    VkPipelineLayout standardPipelineLayout = VK_NULL_HANDLE;
    VkPipelineLayout gbufferPipelineLayout = VK_NULL_HANDLE;
    VkPipelineLayout debugPipelineLayout = VK_NULL_HANDLE;

    std::unique_ptr<UniformBuffer> uniformBuffer;
    UniformBufferObject uniformBufferObject{};

    bool swapchainOutOfDate = false;
    bool initialized = false;

    static Camera camera;

    Asset testAsset;
};

#endif // MERIDIAN_RENDERER_H