//
// Created by Dmitri Wamback on 2026-05-30.
//

#ifndef MERIDIAN_MEMORY_INTERNAL_H
#define MERIDIAN_MEMORY_INTERNAL_H

#include <vulkan/vulkan.h>
#include <vector>
#include <optional>
#include <string>
#include <set>

#include "../VulkanResources.h"

namespace Meridian {

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentQueue;

    bool isComplete() const {
        return graphicsFamily.has_value() && presentQueue.has_value();
    }
};

struct SwapchainDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct MSAA {
    VkImage         image = VK_NULL_HANDLE;
    VkDeviceMemory  mem = VK_NULL_HANDLE;
    VkImageView     view = VK_NULL_HANDLE;
};

class Internal {
public:
    // Instance creation
    static VkInstance CreateInstance(
        const std::string& appName,
        const std::string& engineName,
        uint32_t appVersion,
        uint32_t engineVersion,
        uint32_t apiVersion,
        VulkanResources& resources
    );

    static void DestroyInstance(VkInstance instance);

    static VkPhysicalDevice PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);

    static QueueFamilyIndices FindQueueFamily(VkPhysicalDevice device, VkSurfaceKHR surface);
    static SwapchainDetails QuerySwapchainDetails(VkPhysicalDevice device, VkSurfaceKHR surface);
    static bool CheckDeviceExtensionSupport(VkPhysicalDevice device);

    static void CreateDevice(
        VkPhysicalDevice physicalDevice,
        uint32_t graphicsQueueFamily,
        uint32_t presentQueueFamily,
        VkDevice& device,
        VkQueue& graphicsQueue,
        VkQueue& presentQueue
    );

    static void DestroyDevice(VkDevice device);

    static void CreateSwapchain(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkSurfaceKHR surface,
        uint32_t graphicsQueueFamily,
        uint32_t presentQueueFamily,
        VkExtent2D windowExtent,
        VkSwapchainKHR& swapchain,
        std::vector<VkImage>& swapchainImages,
        std::vector<VkImageView>& swapchainImageViews,
        VkFormat& swapchainImageFormat,
        VkExtent2D& swapchainExtent
    );

    static void DestroySwapchain(VkDevice device, VkSwapchainKHR swapchain);

    static void CreateRenderPass(
        VkDevice device,
        VkFormat swapchainImageFormat,
        VkFormat depthFormat,
        VkRenderPass& renderPass
    );

    static void DestroyRenderPass(VkDevice device, VkRenderPass renderPass);

    static void CreateFramebuffers(
        VkDevice device,
        VkRenderPass renderPass,
        const std::vector<VkImageView>& swapchainImageViews,
        VkExtent2D swapchainExtent,
        std::vector<VkFramebuffer>& framebuffers,
        VkImageView depthImageView
    );

    static void DestroyFramebuffers(VkDevice device, std::vector<VkFramebuffer>& framebuffers);

    static void CreateCommandPool(
        VkDevice device,
        uint32_t queueFamily,
        VkCommandPool& commandPool
    );

    static void DestroyCommandPool(VkDevice device, VkCommandPool commandPool);

    static void CreateCommandBuffers(
        VkDevice device,
        VkCommandPool commandPool,
        std::vector<VkCommandBuffer>& commandBuffers,
        uint32_t count = IN_FLIGHT_FRAMES
    );

    static void DestroyCommandBuffers(VkDevice device, VkCommandPool commandPool,
                                      std::vector<VkCommandBuffer>& commandBuffers);

    static void CreateSyncObjects(
        VkDevice device,
        std::vector<VkSemaphore>& imageAvailableSemaphores,
        std::vector<VkSemaphore>& renderCompleteSemaphores,
        std::vector<VkFence>& inFlightFences,
        uint32_t count = IN_FLIGHT_FRAMES
    );

    static void DestroySyncObjects(VkDevice device,
                                   std::vector<VkSemaphore>& imageAvailableSemaphores,
                                   std::vector<VkSemaphore>& renderCompleteSemaphores,
                                   std::vector<VkFence>& inFlightFences);

    static VkSampleCountFlagBits GetSampleCount(VkPhysicalDevice physicalDevice);
    static VkFormat FindDepthFormat(VkPhysicalDevice physicalDevice);
    static VkFormat FindSupportedFormat(
        VkPhysicalDevice physicalDevice,
        const std::vector<VkFormat>& candidates,
        VkImageTiling tiling,
        VkFormatFeatureFlags features
    );
    static bool HasStencilComponent(VkFormat format);

    static void CreateImage(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        uint32_t width, uint32_t height,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkImage& image,
        VkDeviceMemory& imageMemory,
        uint32_t arrayLayers = 1
    );

    static void DestroyImage(VkDevice device, VkImage image, VkDeviceMemory imageMemory);

    static VkImageView CreateImageView(
        VkDevice device,
        VkImage image,
        VkFormat format,
        VkImageAspectFlags flags,
        VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D,
        uint32_t numLayers = 1,
        uint32_t arrayLayer = 0
    );

    static void DestroyImageView(VkDevice device, VkImageView imageView);

    static void ImageLayoutTransition(
        VkCommandBuffer commandBuffer,
        VkImage image,
        VkFormat format,
        VkImageLayout oldLayout,
        VkImageLayout newLayout
    );

    static void CreateBuffer(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkDeviceSize size,
        VkBufferUsageFlags usageFlags,
        VkMemoryPropertyFlags properties,
        VkBuffer& buffer,
        VkDeviceMemory& bufferMemory
    );

    static void DestroyBuffer(VkDevice device, VkBuffer buffer, VkDeviceMemory bufferMemory);

    static void MemCopyBuffer(
        VkCommandBuffer commandBuffer,
        VkBuffer src,
        VkBuffer dst,
        VkDeviceSize size
    );

    static void MemCopyBufferToImage(
        VkCommandBuffer commandBuffer,
        VkBuffer buffer,
        VkImage image,
        uint32_t width,
        uint32_t height
    );

    static VkCommandBuffer BeginSingleTimeCommand(VkDevice device, VkCommandPool commandPool);

    static void EndSingleTimeCommand(
        VkDevice device,
        VkCommandPool commandPool,
        VkQueue graphicsQueue,
        VkCommandBuffer commandBuffer,
        VkFence fence = VK_NULL_HANDLE
    );

    static uint32_t FindMemoryType(
        VkPhysicalDevice physicalDevice,
        uint32_t typeFilter,
        VkMemoryPropertyFlags properties
    );

    static void CreateDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout& descriptorSetLayout);
    static void CreateDescriptorPool(VkDevice device, uint32_t maxSets, VkDescriptorPool& descriptorPool);

    static void CreateDescriptorSets(
        VkDevice device,
        VkDescriptorPool descriptorPool,
        VkDescriptorSetLayout descriptorSetLayout,
        uint32_t maxSets,
        std::vector<VkDescriptorSet>& descriptorSets
    );

    static void UpdateDescriptorSets(
        VkDevice device,
        VkDescriptorSet descriptorSet,
        VkBuffer uniformBuffer,
        VkDeviceSize uniformBufferSize,
        VkImage textureImage,
        VkImageView textureImageView,
        VkSampler textureSampler
    );

    static void CreateDepthImage(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkExtent2D extent,
        VkFormat depthFormat,
        VkImage& depthImage,
        VkDeviceMemory& depthImageMemory,
        VkImageView& depthImageView
    );

    static void BloomImageLayoutTransition(
        VkCommandBuffer commandBuffer,
        VkImage image,
        uint32_t mipLevel,
        VkImageLayout oldLayout,
        VkImageLayout newLayout
    );

private:
    static bool CheckValidationLayerSupport();
    static bool IsPhysicalDeviceSuitable(
        VkPhysicalDevice device,
        VkSurfaceKHR surface
    );
    static std::vector<const char*> GetRequiredInstanceExtensions();

    static const std::vector<const char*> VALIDATION_LAYERS;
    static const std::vector<const char*> DEVICE_EXTENSIONS;
};

} // namespace Meridian

#endif // MERIDIAN_MEMORY_INTERNAL_H