//
// Created by Dmitri Wamback on 2026-05-30.
//

#include "Internal.h"
#include <iostream>
#include <cstring>
#include <limits>
#include <stdexcept>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Meridian {

    #if defined(__APPLE__)
        constexpr bool Internal::ENABLE_VALIDATION_LAYERS;
    #else
        constexpr bool Internal::ENABLE_VALIDATION_LAYERS;
    #endif

    const std::vector<const char*> Internal::VALIDATION_LAYERS = {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> Internal::DEVICE_EXTENSIONS = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    bool Internal::CheckValidationLayerSupport() {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : VALIDATION_LAYERS) {
            bool found = false;
            for (const auto& layerProperties : availableLayers) {
                if (std::strcmp(layerName, layerProperties.layerName) == 0) {
                    found = true;
                    break;
                }
            }
            if (!found) return false;
        }
        return true;
    }

    std::vector<const char*> Internal::GetRequiredInstanceExtensions() {
        std::vector<const char*> extensions;
#ifdef __APPLE__
        extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#endif

        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        for (uint32_t i = 0; i < glfwExtensionCount; i++) {
            extensions.push_back(glfwExtensions[i]);
        }

        return extensions;
    }


    VkInstance Internal::CreateInstance(
        const std::string& appName,
        const std::string& engineName,
        uint32_t appVersion,
        uint32_t engineVersion,
        uint32_t apiVersion
    ) {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = appName.c_str();
        appInfo.applicationVersion = appVersion;
        appInfo.pEngineName = engineName.c_str();
        appInfo.engineVersion = engineVersion;
        appInfo.apiVersion = apiVersion;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        auto extensions = GetRequiredInstanceExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

    #ifdef __APPLE__
        createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    #endif

        if (ENABLE_VALIDATION_LAYERS) {
            if (!CheckValidationLayerSupport()) {
                std::cerr << "Warning: Validation layers requested but not available!" << std::endl;
            }
            createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
            createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
        }
        else {
            createInfo.enabledLayerCount = 0;
        }

        VkInstance instance;
        VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Vulkan instance! Error code: " + std::to_string(result));
        }

        std::cout << "Vulkan instance created successfully for " << appName << std::endl;
        return instance;
    }

    void Internal::DestroyInstance(VkInstance instance) {
        vkDestroyInstance(instance, nullptr);
    }

    QueueFamilyIndices Internal::FindQueueFamily(VkPhysicalDevice device, VkSurfaceKHR surface) {
        QueueFamilyIndices indices;

        uint32_t familyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, nullptr);
        std::vector<VkQueueFamilyProperties> families(familyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, families.data());

        for (uint32_t i = 0; i < familyCount; i++) {
            if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if (presentSupport) {
                indices.presentQueue = i;
            }

            if (indices.isComplete()) break;
        }

        return indices;
    }

    SwapchainDetails Internal::QuerySwapchainDetails(VkPhysicalDevice device, VkSurfaceKHR surface) {
        SwapchainDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        uint32_t formatCount, presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        return details;
    }

    bool Internal::CheckDeviceExtensionSupport(VkPhysicalDevice device) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());

        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    bool Internal::IsPhysicalDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) {
        SwapchainDetails details = QuerySwapchainDetails(device, surface);
        QueueFamilyIndices family = FindQueueFamily(device, surface);

        bool adequate = !details.formats.empty() && !details.presentModes.empty();
        return family.isComplete() && adequate;
    }

    VkPhysicalDevice Internal::PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface) {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        if (deviceCount == 0) {
            throw std::runtime_error("Failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        for (auto device : devices) {
            if (IsPhysicalDeviceSuitable(device, surface)) {
                return device;
            }
        }

        throw std::runtime_error("Failed to find a suitable GPU!");
    }

    void Internal::CreateDevice(
        VkPhysicalDevice physicalDevice,
        uint32_t graphicsQueueFamily,
        uint32_t presentQueueFamily,
        VkDevice& device,
        VkQueue& graphicsQueue,
        VkQueue& presentQueue
    ) {
        QueueFamilyIndices indices;
        indices.graphicsFamily = graphicsQueueFamily;
        indices.presentQueue = presentQueueFamily;

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        float queuePriority = 1.0f;

        std::set<uint32_t> uniqueQueueFamilies = {
            indices.graphicsFamily.value(),
            indices.presentQueue.value()
        };

        for (uint32_t qFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = qFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures features{};
        features.multiDrawIndirect = VK_TRUE;

        VkDeviceCreateInfo deviceInfo{};
        deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
        deviceInfo.pEnabledFeatures = &features;
        deviceInfo.enabledExtensionCount = static_cast<uint32_t>(DEVICE_EXTENSIONS.size());
        deviceInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();

        if (ENABLE_VALIDATION_LAYERS) {
            deviceInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
            deviceInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
        }
        else {
            deviceInfo.enabledLayerCount = 0;
        }

        if (vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &device) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create logical device!");
        }

        vkGetDeviceQueue(device, graphicsQueueFamily, 0, &graphicsQueue);
        vkGetDeviceQueue(device, presentQueueFamily, 0, &presentQueue);
    }

    void Internal::DestroyDevice(VkDevice device) {
        vkDestroyDevice(device, nullptr);
    }

    void Internal::CreateSwapchain(
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
    ) {
        SwapchainDetails details = QuerySwapchainDetails(physicalDevice, surface);

        auto chooseSurfaceFormat = [](const std::vector<VkSurfaceFormatKHR>& availableFormats) {
            for (const auto& format : availableFormats) {
                if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
                    format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                    return format;
                }
            }
            return availableFormats[0];
        };

        auto choosePresentMode = [](const std::vector<VkPresentModeKHR>& availablePresentModes) {
            for (const auto& presentMode : availablePresentModes) {
                if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) return presentMode;
            }
            return VK_PRESENT_MODE_FIFO_KHR;
        };

        auto chooseSwapchainExtent = [&](const VkSurfaceCapabilitiesKHR& capabilities) {
            if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
                return capabilities.currentExtent;
            }

            VkExtent2D actualExtent = windowExtent;
            actualExtent.width = std::clamp(actualExtent.width,
                                            capabilities.minImageExtent.width,
                                            capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height,
                                             capabilities.minImageExtent.height,
                                             capabilities.maxImageExtent.height);

            return actualExtent;
        };

        VkSurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(details.formats);
        VkPresentModeKHR presentMode = choosePresentMode(details.presentModes);
        VkExtent2D extent = chooseSwapchainExtent(details.capabilities);

        swapchainImageFormat = surfaceFormat.format;
        swapchainExtent = extent;

        uint32_t imageCount = details.capabilities.minImageCount + 1;
        if (details.capabilities.maxImageCount > 0 && imageCount > details.capabilities.maxImageCount) {
            imageCount = details.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR swapchainInfo{};
        swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainInfo.surface = surface;
        swapchainInfo.minImageCount = imageCount;
        swapchainInfo.imageFormat = surfaceFormat.format;
        swapchainInfo.imageColorSpace = surfaceFormat.colorSpace;
        swapchainInfo.imageExtent = extent;
        swapchainInfo.imageArrayLayers = 1;
        swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        if (graphicsQueueFamily != presentQueueFamily) {
            uint32_t queueFamilyIndices[] = {graphicsQueueFamily, presentQueueFamily};
            swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            swapchainInfo.queueFamilyIndexCount = 2;
            swapchainInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else {
            swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        swapchainInfo.preTransform = details.capabilities.currentTransform;
        swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainInfo.presentMode = presentMode;
        swapchainInfo.clipped = VK_TRUE;
        swapchainInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(device, &swapchainInfo, nullptr, &swapchain) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create swapchain!");
        }

        vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
        swapchainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());

        swapchainImageViews.resize(swapchainImages.size());
        for (size_t i = 0; i < swapchainImages.size(); i++) {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = swapchainImages[i];
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = swapchainImageFormat;
            viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(device, &viewInfo, nullptr, &swapchainImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create image views!");
            }
        }
    }

    void Internal::DestroySwapchain(VkDevice device, VkSwapchainKHR swapchain) {
        vkDestroySwapchainKHR(device, swapchain, nullptr);
    }

    void Internal::CreateRenderPass(
        VkDevice device,
        VkFormat swapchainImageFormat,
        VkRenderPass& renderPass
    ) {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapchainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorRef{};
        colorRef.attachment = 0;
        colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        createInfo.attachmentCount = 1;
        createInfo.pAttachments = &colorAttachment;
        createInfo.subpassCount = 1;
        createInfo.pSubpasses = &subpass;
        createInfo.dependencyCount = 1;
        createInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(device, &createInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create render pass!");
        }
    }

    void Internal::DestroyRenderPass(VkDevice device, VkRenderPass renderPass) {
        vkDestroyRenderPass(device, renderPass, nullptr);
    }

    void Internal::CreateFramebuffers(
        VkDevice device,
        VkRenderPass renderPass,
        const std::vector<VkImageView>& swapchainImageViews,
        VkExtent2D swapchainExtent,
        std::vector<VkFramebuffer>& framebuffers
    ) {
        framebuffers.resize(swapchainImageViews.size());

        for (size_t i = 0; i < swapchainImageViews.size(); i++) {
            VkImageView attachments[] = {swapchainImageViews[i]};

            VkFramebufferCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            createInfo.renderPass = renderPass;
            createInfo.attachmentCount = 1;
            createInfo.pAttachments = attachments;
            createInfo.width = swapchainExtent.width;
            createInfo.height = swapchainExtent.height;
            createInfo.layers = 1;

            if (vkCreateFramebuffer(device, &createInfo, nullptr, &framebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create framebuffer!");
            }
        }
    }

    void Internal::DestroyFramebuffers(VkDevice device, std::vector<VkFramebuffer>& framebuffers) {
        for (auto framebuffer : framebuffers) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
        framebuffers.clear();
    }

    void Internal::CreateCommandPool(
        VkDevice device,
        uint32_t queueFamily,
        VkCommandPool& commandPool
    ) {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamily;

        if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create command pool!");
        }
    }

    void Internal::DestroyCommandPool(VkDevice device, VkCommandPool commandPool) {
        vkDestroyCommandPool(device, commandPool, nullptr);
    }

    void Internal::CreateCommandBuffers(
        VkDevice device,
        VkCommandPool commandPool,
        std::vector<VkCommandBuffer>& commandBuffers,
        uint32_t count
    ) {
        commandBuffers.resize(count);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = count;

        if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate command buffers!");
        }
    }

    void Internal::DestroyCommandBuffers(
        VkDevice device,
        VkCommandPool commandPool,
        std::vector<VkCommandBuffer>& commandBuffers
    ) {
        vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
        commandBuffers.clear();
    }

    void Internal::CreateSyncObjects(
        VkDevice device,
        std::vector<VkSemaphore>& imageAvailableSemaphores,
        std::vector<VkSemaphore>& renderCompleteSemaphores,
        std::vector<VkFence>& inFlightFences,
        uint32_t count
    ) {
        imageAvailableSemaphores.resize(count);
        renderCompleteSemaphores.resize(count);
        inFlightFences.resize(count);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < count; i++) {
            if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderCompleteSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create sync objects for frame " + std::to_string(i) + "!");
            }
        }
    }

    void Internal::DestroySyncObjects(
        VkDevice device,
        std::vector<VkSemaphore>& imageAvailableSemaphores,
        std::vector<VkSemaphore>& renderCompleteSemaphores,
        std::vector<VkFence>& inFlightFences
    ) {
        for (size_t i = 0; i < imageAvailableSemaphores.size(); i++) {
            vkDestroySemaphore(device, renderCompleteSemaphores[i], nullptr);
            vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(device, inFlightFences[i], nullptr);
        }
    }

    VkSampleCountFlagBits Internal::GetSampleCount(VkPhysicalDevice physicalDevice) {
        VkPhysicalDeviceProperties physicalDeviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

        VkSampleCountFlags count = physicalDeviceProperties.limits.framebufferColorSampleCounts &
                                   physicalDeviceProperties.limits.framebufferDepthSampleCounts;

        if (count & VK_SAMPLE_COUNT_64_BIT) return VK_SAMPLE_COUNT_64_BIT;
        if (count & VK_SAMPLE_COUNT_32_BIT) return VK_SAMPLE_COUNT_32_BIT;
        if (count & VK_SAMPLE_COUNT_16_BIT) return VK_SAMPLE_COUNT_16_BIT;
        if (count & VK_SAMPLE_COUNT_8_BIT)  return VK_SAMPLE_COUNT_8_BIT;
        if (count & VK_SAMPLE_COUNT_4_BIT)  return VK_SAMPLE_COUNT_4_BIT;
        if (count & VK_SAMPLE_COUNT_2_BIT)  return VK_SAMPLE_COUNT_2_BIT;

        return VK_SAMPLE_COUNT_1_BIT;
    }

    bool Internal::HasStencilComponent(VkFormat format) {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }

    VkFormat Internal::FindSupportedFormat(
        VkPhysicalDevice physicalDevice,
        const std::vector<VkFormat>& candidates,
        VkImageTiling tiling,
        VkFormatFeatureFlags features
    ) {
        for (VkFormat format : candidates) {
            VkFormatProperties properties;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &properties);

            if (tiling == VK_IMAGE_TILING_LINEAR &&
                (properties.linearTilingFeatures & features) == features) {
                return format;
            }
            else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
                       (properties.optimalTilingFeatures & features) == features) {
                return format;
            }
        }
        throw std::runtime_error("Failed to find supported format!");
    }

    VkFormat Internal::FindDepthFormat(VkPhysicalDevice physicalDevice) {
        return FindSupportedFormat(physicalDevice,
                                   {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                                   VK_IMAGE_TILING_OPTIMAL,
                                   VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }

    uint32_t Internal::FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) &&
                (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("Failed to find suitable memory type!");
    }

    void Internal::CreateImage(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        uint32_t width, uint32_t height,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkImage& image,
        VkDeviceMemory& imageMemory,
        uint32_t arrayLayers
    ) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = arrayLayers;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate image memory!");
        }

        vkBindImageMemory(device, image, imageMemory, 0);
    }

    void Internal::DestroyImage(VkDevice device, VkImage image, VkDeviceMemory imageMemory) {
        vkFreeMemory(device, imageMemory, nullptr);
        vkDestroyImage(device, image, nullptr);
    }

    VkImageView Internal::CreateImageView(
        VkDevice device,
        VkImage image,
        VkFormat format,
        VkImageAspectFlags flags,
        VkImageViewType viewType,
        uint32_t numLayers,
        uint32_t arrayLayer
    ) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = viewType;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = flags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = arrayLayer;
        viewInfo.subresourceRange.layerCount = numLayers;

        VkImageView imageView;
        if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image view!");
        }

        return imageView;
    }

    void Internal::DestroyImageView(VkDevice device, VkImageView imageView) {
        vkDestroyImageView(device, imageView, nullptr);
    }

    void Internal::ImageLayoutTransition(
        VkCommandBuffer commandBuffer,
        VkImage image,
        VkFormat format,
        VkImageLayout oldLayout,
        VkImageLayout newLayout
    ) {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            if (HasStencilComponent(format)) {
                barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        }
        else {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }

        VkPipelineStageFlags srcStage, dstStage;
        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        }
        else {
            throw std::runtime_error("Unsupported layout transition!");
        }

        vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

    void Internal::CreateBuffer(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkDeviceSize size,
        VkBufferUsageFlags usageFlags,
        VkMemoryPropertyFlags properties,
        VkBuffer& buffer,
        VkDeviceMemory& bufferMemory
    ) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usageFlags;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate buffer memory!");
        }

        vkBindBufferMemory(device, buffer, bufferMemory, 0);
    }

    void Internal::DestroyBuffer(VkDevice device, VkBuffer buffer, VkDeviceMemory bufferMemory) {
        vkFreeMemory(device, bufferMemory, nullptr);
        vkDestroyBuffer(device, buffer, nullptr);
    }

    void Internal::MemCopyBuffer(
        VkCommandBuffer commandBuffer,
        VkBuffer src,
        VkBuffer dst,
        VkDeviceSize size
    ) {
        VkBufferCopy copy{};
        copy.size = size;
        vkCmdCopyBuffer(commandBuffer, src, dst, 1, &copy);
    }

    void Internal::MemCopyBufferToImage(
        VkCommandBuffer commandBuffer,
        VkBuffer buffer,
        VkImage image,
        uint32_t width,
        uint32_t height
    ) {
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {width, height, 1};

        vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    }

    VkCommandBuffer Internal::BeginSingleTimeCommand(VkDevice device, VkCommandPool commandPool) {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);
        return commandBuffer;
    }

    void Internal::EndSingleTimeCommand(
        VkDevice device,
        VkCommandPool commandPool,
        VkQueue graphicsQueue,
        VkCommandBuffer commandBuffer,
        VkFence fence
    ) {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, fence);

        if (fence != VK_NULL_HANDLE) {
            vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
        }
        else {
            vkQueueWaitIdle(graphicsQueue);
        }

        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }

    void Internal::CreateDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout &descriptorSetLayout) {

        std::vector<VkDescriptorSetLayoutBinding> bindings(1);

        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        bindings[0].pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor set layout!");
        }
    }

    void Internal::CreateDescriptorPool(VkDevice device, uint32_t maxSets, VkDescriptorPool &descriptorPool) {
        std::vector<VkDescriptorPoolSize> poolSizes(1);
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = maxSets;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = maxSets;
        poolInfo.flags = 0;

        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor pool!");
        }
    }

    void Internal::CreateDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout, uint32_t maxSets, std::vector<VkDescriptorSet> &descriptorSets) {
        std::vector<VkDescriptorSetLayout> layouts(maxSets, descriptorSetLayout);

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = maxSets;
        allocInfo.pSetLayouts = layouts.data();

        descriptorSets.resize(maxSets);

        if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate descriptor sets!");
        }
    }

    void Internal::UpdateDescriptorSets(VkDevice device, VkDescriptorSet descriptorSet, VkBuffer uniformBuffer, VkDeviceSize uniformBufferSize, VkImage textureImage, VkImageView textureImageView, VkSampler textureSampler) {

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffer;
        bufferInfo.offset = 0;
        bufferInfo.range = uniformBufferSize;

        VkWriteDescriptorSet writeUBO{};
        writeUBO.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeUBO.dstSet = descriptorSet;
        writeUBO.dstBinding = 0;
        writeUBO.dstArrayElement = 0;
        writeUBO.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeUBO.descriptorCount = 1;
        writeUBO.pBufferInfo = &bufferInfo;

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = textureImageView;
        imageInfo.sampler = textureSampler;

        VkWriteDescriptorSet writeTexture{};
        writeTexture.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeTexture.dstSet = descriptorSet;
        writeTexture.dstBinding = 1;
        writeTexture.dstArrayElement = 0;
        writeTexture.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeTexture.descriptorCount = 1;
        writeTexture.pImageInfo = &imageInfo;

        std::array<VkWriteDescriptorSet, 2> writes = {writeUBO, writeTexture};
        vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }
} // namespace Meridian