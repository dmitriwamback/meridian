//
// Created by Dmitri Wamback on 2026-05-30.
//

#include "Renderer.h"
#include <stdexcept>
#include <iostream>

#include "../memory/Internal.h"
#include "../core/buffer/PushConstants.h"

Camera Renderer::camera;

Renderer::~Renderer() {
}

void Renderer::Initialize(GLFWwindow* window, VulkanResources& vulkan) {
    if (initialized) {
        throw std::runtime_error("Renderer already initialized!");
    }

    camera = Camera();
    camera.Initialize();

    testAsset = Asset();
    testAsset.Create();
    testAsset.CreateBuffers(vulkan.device, vulkan.physicalDevice);

    CreatePipelineLayouts(vulkan);
    CreateHDRSampler(vulkan);

    CreateHDRRenderPass(vulkan);
    CreateHDRFramebuffer(vulkan);

    pipelineManager.CreateStandardPipeline(
        "standard",
        vulkan.device,
        hdrRenderPass,
        standardPipelineLayout,
        "/Users/dmitriwamback/CLionProjects/meridian/shaders/standard_vertex.spv",
        "/Users/dmitriwamback/CLionProjects/meridian/shaders/standard_fragment.spv"
    );

    pipelineManager.CreateStandardPipelineDebug(
        "debug",
        vulkan.device,
        vulkan.renderPass,
        debugPipelineLayout
    );


    pipelineManager.CreateBloomDownsamplePipeline(
        "bloom_downsample",
        vulkan.device,
        bloomPipelineLayout,
        "/Users/dmitriwamback/CLionProjects/meridian/shaders/bloom_downsample.spv"
    );

    pipelineManager.CreateBloomUpsamplePipeline(
        "bloom_upsample",
        vulkan.device,
        bloomPipelineLayout,
        "/Users/dmitriwamback/CLionProjects/meridian/shaders/bloom_upsample.spv"
    );

    CreateBloomImages(vulkan);

    pipelineManager.CreateCompositePipeline(
        "composite",
        vulkan.device,
        vulkan.renderPass,
        compositePipelineLayout,
    "/Users/dmitriwamback/CLionProjects/meridian/shaders/composite_vertex.spv",
"/Users/dmitriwamback/CLionProjects/meridian/shaders/composite_fragment.spv"
    );

    CreateCompositeDescriptorSet(vulkan);

    /*
    pipelineManager.CreateGBufferPipeline(
        "gbuffer",
        vulkan.device,
        vulkan.renderPass,
        gbufferPipelineLayout,
        "/Users/dmitriwamback/CLionProjects/meridian/shaders/gbuffer_vertex.spv",
        "/Users/dmitriwamback/CLionProjects/meridian/shaders/gbuffer_fragment.spv"
    );
    */

    initialized = true;
    std::cout << "Renderer initialized successfully!" << std::endl;
}

void Renderer::CreatePipelineLayouts(VulkanResources& vulkan) {

    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(vulkan.physicalDevice, &memoryProperties);

    uniformBuffer = std::make_unique<UniformBuffer>(
        vulkan.device,
        memoryProperties,
        sizeof(UniformBufferObject)
    );

    UpdateUniformBuffers(vulkan);

    {
        std::vector<VkDescriptorSetLayout> setLayouts(1);
        setLayouts[0] = vulkan.descriptorSetLayout;

        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(PushConstants);

        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
        layoutInfo.pSetLayouts = setLayouts.data();
        layoutInfo.pushConstantRangeCount = 1;
        layoutInfo.pPushConstantRanges = &pushConstantRange;

        if (vkCreatePipelineLayout(vulkan.device, &layoutInfo, nullptr, &standardPipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create standard pipeline layout with push constants");
        }
    }

    {
        std::vector<VkDescriptorSetLayout> setLayouts(1);
        setLayouts[0] = vulkan.descriptorSetLayout;

        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
        layoutInfo.pSetLayouts = setLayouts.data();

        if (vkCreatePipelineLayout(vulkan.device, &layoutInfo, nullptr, &debugPipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create debug pipeline layout!");
        }
    }

    {
        std::vector<VkDescriptorSetLayout> setLayouts(1);
        setLayouts[0] = vulkan.descriptorSetLayout;

        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
        layoutInfo.pSetLayouts = setLayouts.data();

        if (vkCreatePipelineLayout(vulkan.device, &layoutInfo, nullptr, &gbufferPipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create G-buffer pipeline layout!");
        }
    }

    {
        std::vector<VkDescriptorSetLayoutBinding> set0Bindings(2);

        set0Bindings[0].binding = 0;
        set0Bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        set0Bindings[0].descriptorCount = 1;
        set0Bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        set0Bindings[1].binding = 1;
        set0Bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        set0Bindings[1].descriptorCount = 1;
        set0Bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutCreateInfo set0LayoutInfo{};
        set0LayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        set0LayoutInfo.bindingCount = static_cast<uint32_t>(set0Bindings.size());
        set0LayoutInfo.pBindings = set0Bindings.data();

        if (vkCreateDescriptorSetLayout(vulkan.device, &set0LayoutInfo, nullptr, &bloomDescriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create bloom descriptor set layout 0!");
        }

        std::vector<VkDescriptorSetLayoutBinding> set1Bindings(1);
        set1Bindings[0].binding = 0;
        set1Bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        set1Bindings[0].descriptorCount = 1;
        set1Bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutCreateInfo set1LayoutInfo{};
        set1LayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        set1LayoutInfo.bindingCount = static_cast<uint32_t>(set1Bindings.size());
        set1LayoutInfo.pBindings = set1Bindings.data();

        if (vkCreateDescriptorSetLayout(vulkan.device, &set1LayoutInfo, nullptr, &bloomDescriptorSetLayout2) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create bloom descriptor set layout 1!");
        }

        std::vector<VkDescriptorSetLayout> setLayouts(2);
        setLayouts[0] = bloomDescriptorSetLayout;
        setLayouts[1] = bloomDescriptorSetLayout2;

        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
        layoutInfo.pSetLayouts = setLayouts.data();

        if (vkCreatePipelineLayout(vulkan.device, &layoutInfo, nullptr, &bloomPipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create bloom pipeline layout!");
        }
    }

    {
        std::vector<VkDescriptorSetLayoutBinding> set0Bindings(2);

        set0Bindings[0].binding = 0;
        set0Bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        set0Bindings[0].descriptorCount = 1;
        set0Bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        set0Bindings[1].binding = 1;
        set0Bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        set0Bindings[1].descriptorCount = 1;
        set0Bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo set0LayoutInfo{};
        set0LayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        set0LayoutInfo.bindingCount = static_cast<uint32_t>(set0Bindings.size());
        set0LayoutInfo.pBindings = set0Bindings.data();

        if (vkCreateDescriptorSetLayout(vulkan.device, &set0LayoutInfo, nullptr, &compositeDescriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create composite descriptor set layout!");
        }

        std::vector<VkDescriptorSetLayout> setLayouts(1);
        setLayouts[0] = compositeDescriptorSetLayout;

        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
        layoutInfo.pSetLayouts = setLayouts.data();

        if (vkCreatePipelineLayout(vulkan.device, &layoutInfo, nullptr, &compositePipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create composite pipeline layout!");
        }
    }

    {
        std::vector<VkDescriptorPoolSize> poolSizes(1);
        poolSizes[0].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[0].descriptorCount = 2 * IN_FLIGHT_FRAMES;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes    = poolSizes.data();
        poolInfo.maxSets       = IN_FLIGHT_FRAMES;

        if (vkCreateDescriptorPool(vulkan.device, &poolInfo, nullptr, &compositeDescriptorPool) != VK_SUCCESS)
            throw std::runtime_error("Failed to create composite descriptor pool!");
    }
}

void Renderer::RecreateSwapchain(VulkanResources& vulkan) {
    int width = 0, height = 0;
    glfwGetFramebufferSize(vulkan.window, &width, &height);
    if (width == 0 || height == 0) {
        return;
    }

    vkDeviceWaitIdle(vulkan.device);
    swapchainOutOfDate = false;

    Meridian::Internal::CreateSwapchain(
        vulkan.device,
        vulkan.physicalDevice,
        vulkan.surface,
        vulkan.graphicsQueueFamily,
        vulkan.presentQueueFamily,
        vulkan.windowExtent,
        vulkan.swapchain,
        vulkan.swapchainImages,
        vulkan.swapchainImageViews,
        vulkan.swapchainImageFormat,
        vulkan.swapchainExtent
    );

    Meridian::Internal::CreateFramebuffers(
        vulkan.device,
        vulkan.renderPass,
        vulkan.swapchainImageViews,
        vulkan.swapchainExtent,
        vulkan.framebuffers,
        vulkan.depthImageView
    );
}

void Renderer::Cleanup(VulkanResources& vulkan) {
    if (!initialized) return;

    vkDeviceWaitIdle(vulkan.device);

    pipelineManager.Cleanup(vulkan.device);
    CleanupBloomImages(vulkan);
    CleanupHDRResources(vulkan);

    if (standardPipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(vulkan.device, standardPipelineLayout, nullptr);
        standardPipelineLayout = VK_NULL_HANDLE;
    }

    if (gbufferPipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(vulkan.device, gbufferPipelineLayout, nullptr);
        gbufferPipelineLayout = VK_NULL_HANDLE;
    }

    if (debugPipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(vulkan.device, debugPipelineLayout, nullptr);
        debugPipelineLayout = VK_NULL_HANDLE;
    }

    if (bloomPipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(vulkan.device, bloomPipelineLayout, nullptr);
        bloomPipelineLayout = VK_NULL_HANDLE;
    }

    if (bloomDescriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(vulkan.device, bloomDescriptorSetLayout, nullptr);
    }
    if (bloomDescriptorSetLayout2 != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(vulkan.device, bloomDescriptorSetLayout2, nullptr);
    }

    if (compositePipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(vulkan.device, compositePipelineLayout, nullptr);
        compositePipelineLayout = VK_NULL_HANDLE;
    }

    if (compositeDescriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(vulkan.device, compositeDescriptorSetLayout, nullptr);
        compositeDescriptorSetLayout = VK_NULL_HANDLE;
    }

    initialized = false;
}

bool Renderer::IsSwapchainOutOfRange() const {
    return swapchainOutOfDate;
}

PipelineManager& Renderer::GetPipelineManager() {
    return pipelineManager;
}

const PipelineManager& Renderer::GetPipelineManager() const {
    return pipelineManager;
}

void Renderer::CursorPosCallback(GLFWwindow *window, double xpos, double ypos) {
    if (glfwGetMouseButton(window, camera.mouseButton) == GLFW_PRESS) {

        float deltaX = xpos - camera.lastMouseX;
        float deltaY = ypos - camera.lastMouseY;

        camera.pitch -= deltaY * 0.005f;
        camera.yaw += deltaX * 0.005f;

        if (camera.pitch > 1.55f) camera.pitch = 1.55f;
        if (camera.pitch < -1.55f) camera.pitch = -1.55f;

        camera.lookDirection = glm::normalize(glm::vec3(cos(camera.yaw) * cos(camera.pitch),
                                                          sin(camera.pitch),
                                                          sin(camera.yaw) * cos(camera.pitch)));
    }

    camera.lastMouseX = xpos;
    camera.lastMouseY = ypos;
}

void Renderer::UpdateUniformBuffers(VulkanResources& vulkan) {

    uniformBufferObject.model = glm::mat4(1.0f);
    uniformBufferObject.view = camera.view;
    uniformBufferObject.proj = camera.projection;

    uniformBuffer->Update(&uniformBufferObject, sizeof(uniformBufferObject));

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = uniformBuffer->GetBuffer();
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = vulkan.descriptorSets[vulkan.currentFrame];
    write.dstBinding = 0;
    write.dstArrayElement = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.descriptorCount = 1;
    write.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(vulkan.device, 1, &write, 0, nullptr);
}

void Renderer::CreateBloomImages(VulkanResources& vulkan) {

    VkDevice         device         = vulkan.device;
    VkPhysicalDevice physicalDevice = vulkan.physicalDevice;
    VkExtent2D       extent         = vulkan.swapchainExtent;

    bloomBaseExtent = extent;

    {
        uint32_t fullLevels = static_cast<uint32_t>(
            std::floor(std::log2(std::max(extent.width, extent.height)))) + 1;
        bloomMipLevels = fullLevels;
        while (bloomMipLevels > 2) {
            uint32_t w = std::max(extent.width  >> (bloomMipLevels - 1), 1u);
            uint32_t h = std::max(extent.height >> (bloomMipLevels - 1), 1u);
            if (w >= 8 && h >= 8) break;
            --bloomMipLevels;
        }
        bloomMipLevels = std::min(bloomMipLevels, 7u);
    }

    if (bloomMipLevels < 2)
        throw std::runtime_error("Bloom requires at least 2 mip levels.");

    uint32_t pairCount = bloomMipLevels - 1;

    VkFormat bloomFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

    bloomImages.clear();
    bloomImageMemories.clear();
    bloomImageViews.clear();
    bloomMipLayouts.clear();

    bloomImages.resize(IN_FLIGHT_FRAMES);
    bloomImageMemories.resize(IN_FLIGHT_FRAMES);
    bloomImageViews.resize(IN_FLIGHT_FRAMES);
    bloomMipLayouts.resize(IN_FLIGHT_FRAMES);

    for (uint32_t frame = 0; frame < IN_FLIGHT_FRAMES; frame++) {
        bloomImages[frame].resize(bloomMipLevels, VK_NULL_HANDLE);
        bloomImageMemories[frame].resize(bloomMipLevels, VK_NULL_HANDLE);
        bloomImageViews[frame].resize(bloomMipLevels, VK_NULL_HANDLE);
        bloomMipLayouts[frame].assign(bloomMipLevels, VK_IMAGE_LAYOUT_UNDEFINED);
    }

    if (bloomDescriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, bloomDescriptorPool, nullptr);
        bloomDescriptorPool = VK_NULL_HANDLE;
    }

    uint32_t imagePairs   = 2 * IN_FLIGHT_FRAMES * pairCount;
    uint32_t uniformPairs = 2 * IN_FLIGHT_FRAMES * pairCount;

    std::vector<VkDescriptorPoolSize> poolSizes = {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imagePairs   },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          imagePairs   },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         uniformPairs },
    };

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes    = poolSizes.data();
    poolInfo.maxSets       = imagePairs + uniformPairs;

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &bloomDescriptorPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create bloom descriptor pool!");

    for (uint32_t frame = 0; frame < IN_FLIGHT_FRAMES; frame++) {
        for (uint32_t i = 0; i < bloomMipLevels; ++i) {
            VkExtent3D imageExtent{
                std::max(extent.width  >> i, 1u),
                std::max(extent.height >> i, 1u),
                1
            };

            VkImageCreateInfo imageInfo{};
            imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType     = VK_IMAGE_TYPE_2D;
            imageInfo.format        = bloomFormat;
            imageInfo.extent        = imageExtent;
            imageInfo.mipLevels     = 1;
            imageInfo.arrayLayers   = 1;
            imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.usage         = VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                      VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                      VK_IMAGE_USAGE_STORAGE_BIT      |
                                      VK_IMAGE_USAGE_SAMPLED_BIT;
            imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            if (vkCreateImage(device, &imageInfo, nullptr, &bloomImages[frame][i]) != VK_SUCCESS)
                throw std::runtime_error("Failed to create bloom image!");

            VkMemoryRequirements memReq{};
            vkGetImageMemoryRequirements(device, bloomImages[frame][i], &memReq);

            VkPhysicalDeviceMemoryProperties memProps{};
            vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memReq.size;

            bool found = false;
            for (uint32_t j = 0; j < memProps.memoryTypeCount; ++j) {
                if ((memReq.memoryTypeBits & (1u << j)) &&
                    (memProps.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
                    allocInfo.memoryTypeIndex = j;
                    found = true;
                    break;
                }
            }
            if (!found)
                throw std::runtime_error("Failed to find suitable memory type for bloom image!");

            if (vkAllocateMemory(device, &allocInfo, nullptr, &bloomImageMemories[frame][i]) != VK_SUCCESS)
                throw std::runtime_error("Failed to allocate bloom image memory!");
            if (vkBindImageMemory(device, bloomImages[frame][i], bloomImageMemories[frame][i], 0) != VK_SUCCESS)
                throw std::runtime_error("Failed to bind bloom image memory!");

            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image                           = bloomImages[frame][i];
            viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format                          = bloomFormat;
            viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel   = 0;
            viewInfo.subresourceRange.levelCount     = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount     = 1;

            if (vkCreateImageView(device, &viewInfo, nullptr, &bloomImageViews[frame][i]) != VK_SUCCESS)
                throw std::runtime_error("Failed to create bloom image view!");
        }
    }

    CreateBloomUniformBuffers(vulkan);
    InitializeBloomDescriptorSets(vulkan);
}


void Renderer::InitializeBloomDescriptorSets(VulkanResources& vulkan) {
    VkDevice device    = vulkan.device;
    uint32_t pairCount = bloomMipLevels - 1;

    uint32_t setsPerPass = IN_FLIGHT_FRAMES * pairCount;

    bloomDescriptorSetsDownsample.resize(setsPerPass);
    bloomDescriptorSetsUpsample.resize(setsPerPass);

    std::vector<VkDescriptorSetLayout> layouts(setsPerPass, bloomDescriptorSetLayout);

    VkDescriptorSetAllocateInfo ai{};
    ai.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    ai.descriptorPool     = bloomDescriptorPool;
    ai.descriptorSetCount = setsPerPass;
    ai.pSetLayouts        = layouts.data();

    if (vkAllocateDescriptorSets(device, &ai, bloomDescriptorSetsDownsample.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate bloom downsample descriptor sets!");
    if (vkAllocateDescriptorSets(device, &ai, bloomDescriptorSetsUpsample.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate bloom upsample descriptor sets!");

    for (uint32_t frame = 0; frame < IN_FLIGHT_FRAMES; frame++) {
        for (uint32_t i = 0; i < pairCount; ++i) {
            uint32_t idx = frame * pairCount + i;

            VkDescriptorImageInfo inputInfo{};
            inputInfo.sampler     = (i == 0) ? hdrSampler : bloomSampler;
            inputInfo.imageView   = (i == 0) ? hdrColorImageView[frame] : bloomImageViews[frame][i];
            inputInfo.imageLayout = (i == 0) ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                                             : VK_IMAGE_LAYOUT_GENERAL;

            VkDescriptorImageInfo outputInfo{};
            outputInfo.sampler     = VK_NULL_HANDLE;
            outputInfo.imageView   = bloomImageViews[frame][i + 1];
            outputInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

            std::array<VkWriteDescriptorSet, 2> writes{};

            writes[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[0].dstSet          = bloomDescriptorSetsDownsample[idx];
            writes[0].dstBinding      = 0;
            writes[0].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[0].descriptorCount = 1;
            writes[0].pImageInfo      = &inputInfo;

            writes[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[1].dstSet          = bloomDescriptorSetsDownsample[idx];
            writes[1].dstBinding      = 1;
            writes[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            writes[1].descriptorCount = 1;
            writes[1].pImageInfo      = &outputInfo;

            vkUpdateDescriptorSets(device, 2, writes.data(), 0, nullptr);

            VkDescriptorImageInfo srcInfo{};
            srcInfo.sampler     = bloomSampler;
            srcInfo.imageView   = bloomImageViews[frame][i + 1];
            srcInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

            VkDescriptorImageInfo dstInfo{};
            dstInfo.sampler     = VK_NULL_HANDLE;
            dstInfo.imageView   = bloomImageViews[frame][i];
            dstInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

            writes[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[0].dstSet          = bloomDescriptorSetsUpsample[idx];
            writes[0].dstBinding      = 0;
            writes[0].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[0].descriptorCount = 1;
            writes[0].pImageInfo      = &srcInfo;

            writes[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[1].dstSet          = bloomDescriptorSetsUpsample[idx];
            writes[1].dstBinding      = 1;
            writes[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            writes[1].descriptorCount = 1;
            writes[1].pImageInfo      = &dstInfo;

            vkUpdateDescriptorSets(device, 2, writes.data(), 0, nullptr);
        }
    }
}


void Renderer::CleanupBloomImages(VulkanResources& vulkan) {
    vkDeviceWaitIdle(vulkan.device);

    for (uint32_t frame = 0; frame < IN_FLIGHT_FRAMES; frame++) {
        for (auto& imageView : bloomImageViews[frame]) {
            vkDestroyImageView(vulkan.device, imageView, nullptr);
        }
        for (auto& image : bloomImages[frame]) {
            vkDestroyImage(vulkan.device, image, nullptr);
        }
        for (auto& memory : bloomImageMemories[frame]) {
            vkFreeMemory(vulkan.device, memory, nullptr);
        }
    }
    if (bloomDescriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(vulkan.device, bloomDescriptorPool, nullptr);
    }

    bloomImages.clear();
    bloomImageViews.clear();
    bloomImageMemories.clear();
    bloomDescriptorSets.clear();
}

void Renderer::CreateHDRRenderPass(VulkanResources& vulkan) {
    VkDevice device = vulkan.device;

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = vulkan.depthFormat;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::vector<VkAttachmentDescription> attachments = {colorAttachment, depthAttachment};

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &hdrRenderPass) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create HDR render pass!");
    }
}

void Renderer::CreateHDRFramebuffer(VulkanResources& vulkan) {
    VkDevice device = vulkan.device;
    VkPhysicalDevice physicalDevice = vulkan.physicalDevice;
    VkExtent2D extent = vulkan.swapchainExtent;

    hdrColorImageView.resize(IN_FLIGHT_FRAMES);
    hdrColorImageMemory.resize(IN_FLIGHT_FRAMES);
    hdrColorImage.resize(IN_FLIGHT_FRAMES);

    hdrFramebuffers.resize(IN_FLIGHT_FRAMES);

    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = vulkan.depthFormat;
        imageInfo.extent = {extent.width, extent.height, 1};
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        if (vkCreateImage(device, &imageInfo, nullptr, &hdrDepthImage) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create HDR depth image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, hdrDepthImage, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;

        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((memRequirements.memoryTypeBits & (1 << i)) &&
                (memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
                allocInfo.memoryTypeIndex = i;
                break;
            }
        }

        if (vkAllocateMemory(device, &allocInfo, nullptr, &hdrDepthImageMemory) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate HDR depth image memory!");
        }

        if (vkBindImageMemory(device, hdrDepthImage, hdrDepthImageMemory, 0) != VK_SUCCESS) {
            throw std::runtime_error("Failed to bind HDR depth image memory!");
        }

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = hdrDepthImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = vulkan.depthFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &viewInfo, nullptr, &hdrDepthImageView) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create HDR depth image view!");
        }
    }

    for (uint32_t frame = 0; frame < IN_FLIGHT_FRAMES; frame++) {

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
        imageInfo.extent = {extent.width, extent.height, 1};
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                          VK_IMAGE_USAGE_SAMPLED_BIT |
                          VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        vkCreateImage(device, &imageInfo, nullptr, &hdrColorImage[frame]);

        VkMemoryRequirements memReq;
        vkGetImageMemoryRequirements(device, hdrColorImage[frame], &memReq);

        VkPhysicalDeviceMemoryProperties memProps;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);

        VkMemoryAllocateInfo alloc{};
        alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc.allocationSize = memReq.size;

        for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
            if (memReq.memoryTypeBits & (1 << i) &&
                (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
                alloc.memoryTypeIndex = i;
                break;
            }
        }

        vkAllocateMemory(device, &alloc, nullptr, &hdrColorImageMemory[frame]);
        vkBindImageMemory(device, hdrColorImage[frame], hdrColorImageMemory[frame], 0);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = hdrColorImage[frame];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;

        vkCreateImageView(device, &viewInfo, nullptr, &hdrColorImageView[frame]);

        VkImageView attachments[] = {
            hdrColorImageView[frame],
            hdrDepthImageView
        };

        VkFramebufferCreateInfo fb{};
        fb.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fb.renderPass = hdrRenderPass;
        fb.attachmentCount = 2;
        fb.pAttachments = attachments;
        fb.width = extent.width;
        fb.height = extent.height;
        fb.layers = 1;

        vkCreateFramebuffer(device, &fb, nullptr, &hdrFramebuffers[frame]);
    }
}


void Renderer::CreateHDRSampler(VulkanResources& vulkan) {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(vulkan.device, &samplerInfo, nullptr, &hdrSampler) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create HDR sampler!");
    }

    if (vkCreateSampler(vulkan.device, &samplerInfo, nullptr, &bloomSampler) != VK_SUCCESS)
        throw std::runtime_error("Failed to create bloom sampler!");
}







void Renderer::Render(VulkanResources& vulkan) {
    if (!initialized) {
        throw std::runtime_error("Renderer not initialized!");
    }

    glm::vec4 movement(0.0f);
    movement.x = glfwGetKey(vulkan.window, GLFW_KEY_W) == GLFW_PRESS ? 1.0f : 0.0f;
    movement.y = glfwGetKey(vulkan.window, GLFW_KEY_S) == GLFW_PRESS ? -1.0f : 0.0f;
    movement.z = glfwGetKey(vulkan.window, GLFW_KEY_A) == GLFW_PRESS ? 1.0f : 0.0f;
    movement.w = glfwGetKey(vulkan.window, GLFW_KEY_D) == GLFW_PRESS ? -1.0f : 0.0f;

    float up   = glfwGetKey(vulkan.window, GLFW_KEY_E) == GLFW_PRESS ? 1.0f : 0.0f;
    float down = glfwGetKey(vulkan.window, GLFW_KEY_Q) == GLFW_PRESS ? -1.0f : 0.0f;

    camera.Update(movement, up, down, vulkan);

    vkWaitForFences(vulkan.device, 1, &vulkan.inFlightFences[vulkan.currentFrame], VK_TRUE, UINT64_MAX);

    UpdateUniformBuffers(vulkan);

    uint32_t imageIndex = 0;
    VkResult result = vkAcquireNextImageKHR(
        vulkan.device,
        vulkan.swapchain,
        UINT64_MAX,
        vulkan.imageAvailableSemaphores[vulkan.currentFrame],
        VK_NULL_HANDLE,
        &imageIndex
    );

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        swapchainOutOfDate = true;
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swapchain image!");
    }

    vkResetFences(vulkan.device, 1, &vulkan.inFlightFences[vulkan.currentFrame]);
    vkResetCommandBuffer(vulkan.commandBuffers[vulkan.currentFrame], 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(vulkan.commandBuffers[vulkan.currentFrame], &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin recording command buffer!");
    }

    RenderSceneToHDR(vulkan, vulkan.commandBuffers[vulkan.currentFrame]);
    RenderBloom(vulkan, vulkan.commandBuffers[vulkan.currentFrame]);
    RenderComposite(vulkan, vulkan.commandBuffers[vulkan.currentFrame], imageIndex);

    if (vkEndCommandBuffer(vulkan.commandBuffers[vulkan.currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to record command buffer!");
    }

    VkSemaphore          waitSemaphores[] = { vulkan.imageAvailableSemaphores[vulkan.currentFrame] };
    VkPipelineStageFlags waitStages[]     = { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };
    VkSemaphore          signalSemaphores[] = { vulkan.renderCompleteSemaphores[vulkan.currentFrame] };

    VkSubmitInfo submitInfo{};
    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pWaitSemaphores      = waitSemaphores;
    submitInfo.pWaitDstStageMask    = waitStages;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &vulkan.commandBuffers[vulkan.currentFrame];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = signalSemaphores;

    if (vkQueueSubmit(vulkan.graphicsQueue, 1, &submitInfo, vulkan.inFlightFences[vulkan.currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit draw command buffer!");
    }

    VkSwapchainKHR swapchains[] = { vulkan.swapchain };

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = signalSemaphores;
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = swapchains;
    presentInfo.pImageIndices      = &imageIndex;

    result = vkQueuePresentKHR(vulkan.presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        swapchainOutOfDate = true;
    }
    else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swapchain image!");
    }

    vulkan.currentFrame = (vulkan.currentFrame + 1) % IN_FLIGHT_FRAMES;
}


void Renderer::RenderBloom(VulkanResources& vulkan, VkCommandBuffer commandBuffer) {
    if (bloomMipLevels < 2) return;

    VkExtent2D extent    = vulkan.swapchainExtent;
    uint32_t   frame     = vulkan.currentFrame;
    uint32_t   pairCount = bloomMipLevels - 1;

    auto dispatch = [&](uint32_t w, uint32_t h) {
        vkCmdDispatch(commandBuffer, (w + 15) / 16, (h + 15) / 16, 1);
    };

    auto transition = [&](uint32_t mip,
                          VkAccessFlags        srcAccess,
                          VkAccessFlags        dstAccess,
                          VkImageLayout        oldLayout,
                          VkImageLayout        newLayout,
                          VkPipelineStageFlags srcStage,
                          VkPipelineStageFlags dstStage) {
        VkImageMemoryBarrier b{};
        b.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        b.oldLayout                       = oldLayout;
        b.newLayout                       = newLayout;
        b.srcAccessMask                   = srcAccess;
        b.dstAccessMask                   = dstAccess;
        b.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.image                           = bloomImages[frame][mip];
        b.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        b.subresourceRange.baseMipLevel   = 0;
        b.subresourceRange.levelCount     = 1;
        b.subresourceRange.baseArrayLayer = 0;
        b.subresourceRange.layerCount     = 1;
        vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage,
                             0, 0, nullptr, 0, nullptr, 1, &b);
    };

    auto writeUniform = [&](VkDeviceMemory mem, const BloomUniforms& u) {
        void* data = nullptr;
        vkMapMemory(vulkan.device, mem, 0, sizeof(u), 0, &data);
        memcpy(data, &u, sizeof(u));
        vkUnmapMemory(vulkan.device, mem);
    };

    auto* downsamplePipeline = pipelineManager.GetBloomDownsamplePipeline("bloom_downsample");
    auto* upsamplePipeline   = pipelineManager.GetBloomUpsamplePipeline("bloom_upsample");

    {
        std::vector<VkImageMemoryBarrier> barriers(bloomMipLevels);
        for (uint32_t i = 0; i < bloomMipLevels; i++) {
            barriers[i].sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barriers[i].oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
            barriers[i].newLayout                       = VK_IMAGE_LAYOUT_GENERAL;
            barriers[i].srcAccessMask                   = 0;
            barriers[i].dstAccessMask                   = VK_ACCESS_SHADER_READ_BIT |
                                                          VK_ACCESS_SHADER_WRITE_BIT |
                                                          VK_ACCESS_TRANSFER_WRITE_BIT;
            barriers[i].srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            barriers[i].dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            barriers[i].image                           = bloomImages[frame][i];
            barriers[i].subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            barriers[i].subresourceRange.baseMipLevel   = 0;
            barriers[i].subresourceRange.levelCount     = 1;
            barriers[i].subresourceRange.baseArrayLayer = 0;
            barriers[i].subresourceRange.layerCount     = 1;
        }
        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, 0, nullptr, 0, nullptr,
            bloomMipLevels, barriers.data()
        );
    }

    {
        VkClearColorValue clearColor{};
        VkImageSubresourceRange range{};
        range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel   = 0;
        range.levelCount     = 1;
        range.baseArrayLayer = 0;
        range.layerCount     = 1;

        for (uint32_t i = 0; i < bloomMipLevels; i++) {
            vkCmdClearColorImage(commandBuffer, bloomImages[frame][i],
                                 VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &range);
        }

        std::vector<VkImageMemoryBarrier> barriers(bloomMipLevels);
        for (uint32_t i = 0; i < bloomMipLevels; i++) {
            barriers[i].sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barriers[i].oldLayout                       = VK_IMAGE_LAYOUT_GENERAL;
            barriers[i].newLayout                       = VK_IMAGE_LAYOUT_GENERAL;
            barriers[i].srcAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
            barriers[i].dstAccessMask                   = VK_ACCESS_SHADER_READ_BIT |
                                                          VK_ACCESS_SHADER_WRITE_BIT;
            barriers[i].srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            barriers[i].dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            barriers[i].image                           = bloomImages[frame][i];
            barriers[i].subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            barriers[i].subresourceRange.baseMipLevel   = 0;
            barriers[i].subresourceRange.levelCount     = 1;
            barriers[i].subresourceRange.baseArrayLayer = 0;
            barriers[i].subresourceRange.layerCount     = 1;
        }
        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            0, 0, nullptr, 0, nullptr,
            bloomMipLevels, barriers.data()
        );
    }

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, downsamplePipeline->pipeline);

    for (uint32_t i = 0; i < pairCount; i++) {
        uint32_t inW  = std::max(extent.width  >> i,       1u);
        uint32_t inH  = std::max(extent.height >> i,       1u);
        uint32_t outW = std::max(extent.width  >> (i + 1), 1u);
        uint32_t outH = std::max(extent.height >> (i + 1), 1u);

        BloomUniforms u{};
        u.inputResolution  = glm::vec2(inW,  inH);
        u.outputResolution = glm::vec2(outW, outH);
        u.inputTexelSize   = glm::vec2(1.0f / inW, 1.0f / inH);
        u.sourceResolution = glm::vec2(inW,  inH);
        u.sourceTexelSize  = glm::vec2(1.0f / inW, 1.0f / inH);
        u.threshold        = 1.0f;
        u.softKnee         = 0.5f;
        u.filterRadius     = 1.0f;
        u.padding          = 0.0f;
        writeUniform(bloomDownsampleUniformMemories[frame][i], u);

        uint32_t idx = frame * pairCount + i;

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                                bloomPipelineLayout, 0, 1,
                                &bloomDescriptorSetsDownsample[idx], 0, nullptr);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                                bloomPipelineLayout, 1, 1,
                                &bloomDownsampleUniformSets[frame][i], 0, nullptr);

        dispatch(outW, outH);

        transition(i + 1,
                   VK_ACCESS_SHADER_WRITE_BIT,
                   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
                   VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
                   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    }

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, upsamplePipeline->pipeline);

    for (int32_t i = (int32_t)pairCount - 1; i >= 0; i--) {
        uint32_t srcW = std::max(extent.width  >> (i + 1), 1u);
        uint32_t srcH = std::max(extent.height >> (i + 1), 1u);
        uint32_t dstW = std::max(extent.width  >> i,       1u);
        uint32_t dstH = std::max(extent.height >> i,       1u);

        BloomUniforms u{};
        u.inputResolution  = glm::vec2(srcW, srcH);
        u.outputResolution = glm::vec2(dstW, dstH);
        u.inputTexelSize   = glm::vec2(1.0f / srcW, 1.0f / srcH);
        u.sourceResolution = glm::vec2(srcW, srcH);
        u.sourceTexelSize  = glm::vec2(1.0f / srcW, 1.0f / srcH);
        u.threshold        = 1.0f;
        u.softKnee         = 0.5f;
        u.filterRadius     = 1.0f;
        u.padding          = 0.0f;
        writeUniform(bloomUpsampleUniformMemories[frame][i], u);

        uint32_t idx = frame * pairCount + (uint32_t)i;

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                                bloomPipelineLayout, 0, 1,
                                &bloomDescriptorSetsUpsample[idx], 0, nullptr);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                                bloomPipelineLayout, 1, 1,
                                &bloomUpsampleUniformSets[frame][i], 0, nullptr);

        dispatch(dstW, dstH);

        transition((uint32_t)i,
                   VK_ACCESS_SHADER_WRITE_BIT,
                   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
                   VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
                   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    }

    {
        std::vector<VkImageMemoryBarrier> barriers(bloomMipLevels);
        for (uint32_t i = 0; i < bloomMipLevels; i++) {
            barriers[i].sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barriers[i].oldLayout                       = VK_IMAGE_LAYOUT_GENERAL;
            barriers[i].newLayout                       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barriers[i].srcAccessMask                   = VK_ACCESS_SHADER_WRITE_BIT;
            barriers[i].dstAccessMask                   = VK_ACCESS_SHADER_READ_BIT;
            barriers[i].srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            barriers[i].dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            barriers[i].image                           = bloomImages[frame][i];
            barriers[i].subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            barriers[i].subresourceRange.baseMipLevel   = 0;
            barriers[i].subresourceRange.levelCount     = 1;
            barriers[i].subresourceRange.baseArrayLayer = 0;
            barriers[i].subresourceRange.layerCount     = 1;
        }
        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0, 0, nullptr, 0, nullptr,
            bloomMipLevels, barriers.data()
        );
    }
}


void Renderer::RenderSceneToHDR(VulkanResources& vulkan, VkCommandBuffer commandBuffer) {
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass        = hdrRenderPass;
    renderPassInfo.framebuffer       = hdrFramebuffers[vulkan.currentFrame];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = vulkan.swapchainExtent;

    std::vector<VkClearValue> clearValues(2);
    clearValues[0].color        = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues    = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    auto* standardPipeline = pipelineManager.GetStandardPipeline("standard");
    standardPipeline->Bind(commandBuffer);

    VkViewport viewport{};
    viewport.x        = 0.0f;
    viewport.y        = static_cast<float>(vulkan.swapchainExtent.height);
    viewport.width    = static_cast<float>(vulkan.swapchainExtent.width);
    viewport.height   = -static_cast<float>(vulkan.swapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = vulkan.swapchainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        standardPipelineLayout,
        0, 1,
        &vulkan.descriptorSets[vulkan.currentFrame],
        0, nullptr
    );

    VkBuffer     vertexBuffers[] = { testAsset.GetVertexBuffer().GetBuffer() };
    VkDeviceSize offsets[]       = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    testAsset.GetIndexBuffer().Bind(commandBuffer);

    for (int x = 0; x < 30; x++) {
        for (int z = 0; z < 30; z++) {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3((x - 15) * 5, 0.0f, (z - 15) * 5));
            PushConstants pushConstants{};
            pushConstants.model = model;
            vkCmdPushConstants(commandBuffer, standardPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &pushConstants);
            vkCmdDrawIndexed(commandBuffer, testAsset.GetIndexBuffer().GetIndexCount(), 1, 0, 0, 0);
        }
    }

    vkCmdEndRenderPass(commandBuffer);

    VkImageMemoryBarrier hdrBarrier{};
    hdrBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    hdrBarrier.oldLayout                       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    hdrBarrier.newLayout                       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    hdrBarrier.srcAccessMask                   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    hdrBarrier.dstAccessMask                   = VK_ACCESS_SHADER_READ_BIT;
    hdrBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    hdrBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    hdrBarrier.image                           = hdrColorImage[vulkan.currentFrame];
    hdrBarrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    hdrBarrier.subresourceRange.baseMipLevel   = 0;
    hdrBarrier.subresourceRange.levelCount     = 1;
    hdrBarrier.subresourceRange.baseArrayLayer = 0;
    hdrBarrier.subresourceRange.layerCount     = 1;

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &hdrBarrier
    );
}

void Renderer::RenderComposite(VulkanResources& vulkan, VkCommandBuffer commandBuffer, uint32_t imageIndex) {

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass        = vulkan.renderPass;
    renderPassInfo.framebuffer       = vulkan.framebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = vulkan.swapchainExtent;

    std::vector<VkClearValue> clearValues(2);
    clearValues[0].color        = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues    = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    auto* compositePipeline = pipelineManager.GetCompositePipeline("composite");
    compositePipeline->Bind(commandBuffer);

    VkViewport viewport{};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = static_cast<float>(vulkan.swapchainExtent.width);
    viewport.height   = static_cast<float>(vulkan.swapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = vulkan.swapchainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        compositePipelineLayout,
        0, 1,
        &compositeDescriptorSets[vulkan.currentFrame],
        0, nullptr
    );

    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);
}


void Renderer::CleanupHDRResources(VulkanResources& vulkan) {
    vkDeviceWaitIdle(vulkan.device);

    for (int i = 0; i < IN_FLIGHT_FRAMES; i++) {
        if (hdrFramebuffers[i] != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(vulkan.device, hdrFramebuffers[i], nullptr);
        }
    }
    if (hdrRenderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(vulkan.device, hdrRenderPass, nullptr);
        hdrRenderPass = VK_NULL_HANDLE;
    }

    for (int i = 0; i < IN_FLIGHT_FRAMES; i++) {
        if (hdrColorImageView[i] != VK_NULL_HANDLE) {
            vkDestroyImageView(vulkan.device, hdrColorImageView[i], nullptr);
            hdrColorImageView[i] = VK_NULL_HANDLE;
        }
        if (hdrColorImage[i] != VK_NULL_HANDLE) {
            vkDestroyImage(vulkan.device, hdrColorImage[i], nullptr);
            hdrColorImage[i] = VK_NULL_HANDLE;
        }
        if (hdrColorImageMemory[i] != VK_NULL_HANDLE) {
            vkFreeMemory(vulkan.device, hdrColorImageMemory[i], nullptr);
            hdrColorImageMemory[i] = VK_NULL_HANDLE;
        }
    }
    if (hdrDepthImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(vulkan.device, hdrDepthImageView, nullptr);
        hdrDepthImageView = VK_NULL_HANDLE;
    }
    if (hdrDepthImage != VK_NULL_HANDLE) {
        vkDestroyImage(vulkan.device, hdrDepthImage, nullptr);
        hdrDepthImage = VK_NULL_HANDLE;
    }
    if (hdrDepthImageMemory != VK_NULL_HANDLE) {
        vkFreeMemory(vulkan.device, hdrDepthImageMemory, nullptr);
        hdrDepthImageMemory = VK_NULL_HANDLE;
    }
}

void Renderer::CreateBloomUniformBuffers(VulkanResources& vulkan) {
    VkDevice         device         = vulkan.device;
    VkPhysicalDevice physicalDevice = vulkan.physicalDevice;

    uint32_t pairCount = bloomMipLevels - 1;

    bloomDownsampleUniformBuffers.assign(IN_FLIGHT_FRAMES, std::vector<VkBuffer>(pairCount));
    bloomDownsampleUniformMemories.assign(IN_FLIGHT_FRAMES, std::vector<VkDeviceMemory>(pairCount));
    bloomDownsampleUniformSets.assign(IN_FLIGHT_FRAMES, std::vector<VkDescriptorSet>(pairCount));

    bloomUpsampleUniformBuffers.assign(IN_FLIGHT_FRAMES, std::vector<VkBuffer>(pairCount));
    bloomUpsampleUniformMemories.assign(IN_FLIGHT_FRAMES, std::vector<VkDeviceMemory>(pairCount));
    bloomUpsampleUniformSets.assign(IN_FLIGHT_FRAMES, std::vector<VkDescriptorSet>(pairCount));

    uint32_t memTypeIndex = Meridian::Internal::FindMemoryType(
        physicalDevice,
        0xFFFFFFFF,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );

    auto makeBuffer = [&](VkBuffer& buf, VkDeviceMemory& mem) {
        VkBufferCreateInfo bi{};
        bi.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bi.size        = sizeof(BloomUniforms);
        bi.usage       = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        if (vkCreateBuffer(device, &bi, nullptr, &buf) != VK_SUCCESS)
            throw std::runtime_error("Failed to create bloom uniform buffer!");

        VkMemoryRequirements memReq{};
        vkGetBufferMemoryRequirements(device, buf, &memReq);

        VkMemoryAllocateInfo ai{};
        ai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        ai.allocationSize  = memReq.size;
        ai.memoryTypeIndex = Meridian::Internal::FindMemoryType(
            physicalDevice, memReq.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        if (vkAllocateMemory(device, &ai, nullptr, &mem) != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate bloom uniform buffer memory!");
        vkBindBufferMemory(device, buf, mem, 0);
    };

    uint32_t totalSets = IN_FLIGHT_FRAMES * pairCount;

    std::vector<VkDescriptorSetLayout> layouts(totalSets, bloomDescriptorSetLayout2);

    auto allocSets = [&](std::vector<std::vector<VkDescriptorSet>>& sets) {
        std::vector<VkDescriptorSet> flat(totalSets);
        VkDescriptorSetAllocateInfo ai{};
        ai.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        ai.descriptorPool     = bloomDescriptorPool;
        ai.descriptorSetCount = totalSets;
        ai.pSetLayouts        = layouts.data();
        if (vkAllocateDescriptorSets(device, &ai, flat.data()) != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate bloom uniform descriptor sets!");
        for (uint32_t f = 0; f < IN_FLIGHT_FRAMES; f++)
            for (uint32_t i = 0; i < pairCount; i++)
                sets[f][i] = flat[f * pairCount + i];
    };

    allocSets(bloomDownsampleUniformSets);
    allocSets(bloomUpsampleUniformSets);

    for (uint32_t f = 0; f < IN_FLIGHT_FRAMES; f++) {
        for (uint32_t i = 0; i < pairCount; i++) {
            makeBuffer(bloomDownsampleUniformBuffers[f][i], bloomDownsampleUniformMemories[f][i]);
            makeBuffer(bloomUpsampleUniformBuffers[f][i],   bloomUpsampleUniformMemories[f][i]);

            auto writeSet = [&](VkDescriptorSet set, VkBuffer buf) {
                VkDescriptorBufferInfo bi{ buf, 0, sizeof(BloomUniforms) };
                VkWriteDescriptorSet w{};
                w.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                w.dstSet          = set;
                w.dstBinding      = 0;
                w.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                w.descriptorCount = 1;
                w.pBufferInfo     = &bi;
                vkUpdateDescriptorSets(device, 1, &w, 0, nullptr);
            };

            writeSet(bloomDownsampleUniformSets[f][i], bloomDownsampleUniformBuffers[f][i]);
            writeSet(bloomUpsampleUniformSets[f][i],   bloomUpsampleUniformBuffers[f][i]);
        }
    }
}

void Renderer::CreateCompositeDescriptorSet(VulkanResources& vulkan) {

    std::vector<VkDescriptorSetLayout> layouts(IN_FLIGHT_FRAMES, compositeDescriptorSetLayout);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = compositeDescriptorPool;
    allocInfo.descriptorSetCount = IN_FLIGHT_FRAMES;
    allocInfo.pSetLayouts        = layouts.data();

    if (vkAllocateDescriptorSets(vulkan.device, &allocInfo, compositeDescriptorSets) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate composite descriptor sets!");
    }

    for (uint32_t frame = 0; frame < IN_FLIGHT_FRAMES; frame++) {
        VkDescriptorImageInfo hdrInfo{};
        hdrInfo.sampler     = hdrSampler;
        hdrInfo.imageView   = hdrColorImageView[frame];
        hdrInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkDescriptorImageInfo bloomInfo{};
        bloomInfo.sampler     = bloomSampler;
        bloomInfo.imageView   = bloomImageViews[frame][0];
        bloomInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet writes[2]{};

        writes[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet          = compositeDescriptorSets[frame];
        writes[0].dstBinding      = 0;
        writes[0].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[0].descriptorCount = 1;
        writes[0].pImageInfo      = &hdrInfo;

        writes[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet          = compositeDescriptorSets[frame];
        writes[1].dstBinding      = 1;
        writes[1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[1].descriptorCount = 1;
        writes[1].pImageInfo      = &bloomInfo;

        vkUpdateDescriptorSets(vulkan.device, 2, writes, 0, nullptr);
    }
}
