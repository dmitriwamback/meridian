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
    void RenderBloom(VulkanResources& vulkan, VkCommandBuffer commandBuffer);
    void CreateBloomImages(VulkanResources& vulkan);
    void CleanupBloomImages(VulkanResources& vulkan);

    PipelineManager pipelineManager;
    VkPipelineLayout standardPipelineLayout = VK_NULL_HANDLE;
    VkPipelineLayout gbufferPipelineLayout = VK_NULL_HANDLE;
    VkPipelineLayout debugPipelineLayout = VK_NULL_HANDLE;
    VkPipelineLayout bloomPipelineLayout = VK_NULL_HANDLE;
    VkPipelineLayout compositePipelineLayout = VK_NULL_HANDLE;

    VkDescriptorSetLayout bloomDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout bloomDescriptorSetLayout2 = VK_NULL_HANDLE;
    VkDescriptorSetLayout compositeDescriptorSetLayout = VK_NULL_HANDLE;

    std::unique_ptr<UniformBuffer> uniformBuffer;
    UniformBufferObject uniformBufferObject{};

    bool swapchainOutOfDate = false;
    bool initialized = false;

    static Camera camera;

    Asset testAsset;

    std::vector<VkImage> bloomImages;
    std::vector<VkImageView> bloomImageViews;
    std::vector<VkDeviceMemory> bloomImageMemories;
    std::vector<VkDescriptorSet> bloomDescriptorSets;
    uint32_t bloomMipLevels = 5;

    std::vector<VkBuffer> bloomUniformBuffers;
    std::vector<VkDeviceMemory> bloomUniformBufferMemories;

    void CreateBloomUniformBuffers(VulkanResources& vulkan);
    void UpdateBloomUniformBuffers(VulkanResources& vulkan);
    //void CleanupBloomUniformBuffers(VulkanResources& vulkan);

    VkRenderPass hdrRenderPass = VK_NULL_HANDLE;
    VkFramebuffer hdrFramebuffer = VK_NULL_HANDLE;
    VkImage hdrColorImage = VK_NULL_HANDLE;
    VkImageView hdrColorImageView = VK_NULL_HANDLE;
    VkDeviceMemory hdrColorImageMemory = VK_NULL_HANDLE;
    VkImage hdrDepthImage = VK_NULL_HANDLE;
    VkImageView hdrDepthImageView = VK_NULL_HANDLE;
    VkDeviceMemory hdrDepthImageMemory = VK_NULL_HANDLE;
    VkSampler hdrSampler = VK_NULL_HANDLE;
    VkSampler bloomSampler = VK_NULL_HANDLE;

    VkDescriptorSet compositeDescriptorSet = VK_NULL_HANDLE;

    void CreateHDRRenderPass(VulkanResources& vulkan);
    void CreateHDRFramebuffer(VulkanResources& vulkan);
    void CleanupHDRResources(VulkanResources& vulkan);
    void RenderComposite(VulkanResources& vulkan, VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void RenderSceneToHDR(VulkanResources& vulkan, VkCommandBuffer commandBuffer);
    void CreateHDRSampler(VulkanResources& vulkan);

    void CreateCompositeDescriptorSet(VulkanResources& vulkan);

    VkDescriptorPool bloomDescriptorPool = VK_NULL_HANDLE;
    VkDescriptorPool compositeDescriptorPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> bloomUniformDescriptorSets;
};

#endif // MERIDIAN_RENDERER_H