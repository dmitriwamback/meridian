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

    pipelineManager.CreateBloomHorizontalBlurPipeline(
        "bloom_horizontal",
        vulkan.device,
        bloomPipelineLayout,
        "/Users/dmitriwamback/CLionProjects/meridian/shaders/bloom_hblur.spv"
    );

    pipelineManager.CreateBloomVerticalBlurPipeline(
        "bloom_vertical",
        vulkan.device,
        bloomPipelineLayout,
        "/Users/dmitriwamback/CLionProjects/meridian/shaders/bloom_vblur.spv"
    );

    CreateBloomImages(vulkan);
    CreateBloomUniformBuffers(vulkan);

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
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[0].descriptorCount = 2;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = 1;

        if (vkCreateDescriptorPool(vulkan.device, &poolInfo, nullptr, &compositeDescriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create composite descriptor pool!");
        }
    }
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

    UpdateUniformBuffers(vulkan);
    UpdateBloomUniformBuffers(vulkan);

    vkWaitForFences(vulkan.device, 1, &vulkan.inFlightFences[vulkan.currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
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
    beginInfo.flags = 0;

    if (vkBeginCommandBuffer(vulkan.commandBuffers[vulkan.currentFrame], &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin recording command buffer!");
    }

    RenderSceneToHDR(vulkan, vulkan.commandBuffers[vulkan.currentFrame]);

    VkImageMemoryBarrier hdrToSample{};
    hdrToSample.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    hdrToSample.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    hdrToSample.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    hdrToSample.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    hdrToSample.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    hdrToSample.image = hdrColorImage;
    hdrToSample.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    hdrToSample.subresourceRange.baseMipLevel = 0;
    hdrToSample.subresourceRange.levelCount = 1;
    hdrToSample.subresourceRange.baseArrayLayer = 0;
    hdrToSample.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(
        vulkan.commandBuffers[vulkan.currentFrame],
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &hdrToSample
    );


    RenderBloom(vulkan, vulkan.commandBuffers[vulkan.currentFrame]);

    RenderComposite(vulkan, vulkan.commandBuffers[vulkan.currentFrame], imageIndex);

    if (vkEndCommandBuffer(vulkan.commandBuffers[vulkan.currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to recording command buffer!");
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {vulkan.imageAvailableSemaphores[vulkan.currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vulkan.commandBuffers[vulkan.currentFrame];

    VkSemaphore signalSemaphores[] = {vulkan.renderCompleteSemaphores[vulkan.currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(vulkan.graphicsQueue, 1, &submitInfo, vulkan.inFlightFences[vulkan.currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapchains[] = {vulkan.swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(vulkan.presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        swapchainOutOfDate = true;
    }
    else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swapchain image!");
    }

    vulkan.currentFrame = (vulkan.currentFrame + 1) % 2;
}

void Renderer::RenderBloom(VulkanResources& vulkan, VkCommandBuffer commandBuffer) {
    VkExtent2D extent = vulkan.swapchainExtent;

    VkImageCopy copyRegion{};
    copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.srcSubresource.mipLevel = 0;
    copyRegion.srcSubresource.baseArrayLayer = 0;
    copyRegion.srcSubresource.layerCount = 1;
    copyRegion.srcOffset = {0, 0, 0};
    copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.dstSubresource.mipLevel = 0;
    copyRegion.dstSubresource.baseArrayLayer = 0;
    copyRegion.dstSubresource.layerCount = 1;
    copyRegion.dstOffset = {0, 0, 0};
    copyRegion.extent = {extent.width, extent.height, 1};

    VkImageMemoryBarrier hdrTransition{};
    hdrTransition.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    hdrTransition.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    hdrTransition.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    hdrTransition.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    hdrTransition.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    hdrTransition.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    hdrTransition.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    hdrTransition.image = hdrColorImage;
    hdrTransition.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    hdrTransition.subresourceRange.baseMipLevel = 0;
    hdrTransition.subresourceRange.levelCount = 1;
    hdrTransition.subresourceRange.baseArrayLayer = 0;
    hdrTransition.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &hdrTransition);

    vkCmdCopyImage(commandBuffer, hdrColorImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, bloomImages[0], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

    VkImageMemoryBarrier bloomBarrier{};
    bloomBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    bloomBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    bloomBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    bloomBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    bloomBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    bloomBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bloomBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bloomBarrier.image = bloomImages[0];
    bloomBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    bloomBarrier.subresourceRange.baseMipLevel = 0;
    bloomBarrier.subresourceRange.levelCount = 1;
    bloomBarrier.subresourceRange.baseArrayLayer = 0;
    bloomBarrier.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &bloomBarrier);

    auto* downsamplePipeline = pipelineManager.GetBloomDownsamplePipeline("bloom_downsample");
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, downsamplePipeline->pipeline);

    for (uint32_t i = 0; i < bloomMipLevels - 1; i++) {

        uint32_t inW = std::max(extent.width >> i, 1u);
        uint32_t inH = std::max(extent.height >> i, 1u);
        uint32_t outW = std::max(extent.width >> (i + 1), 1u);
        uint32_t outH = std::max(extent.height >> (i + 1), 1u);

        BloomDownsampleUniforms u{};
        u.inputResolution = glm::vec2(inW, inH);
        u.outputResolution = glm::vec2(outW, outH);
        u.inputTexelSize = glm::vec2(1.0f / inW, 1.0f / inH);

        UpdateUniform(vulkan.device, bloomUniformBufferMemories[i], u);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, bloomPipelineLayout, 1, 1, &bloomUniformDescriptorSets[i], 0, nullptr);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, bloomPipelineLayout, 0, 1, &bloomDescriptorSets[i * 2], 0, nullptr);

        uint32_t width = std::max(extent.width >> i, 1u);
        uint32_t height = std::max(extent.height >> i, 1u);
        vkCmdDispatch(commandBuffer, (width + 15) / 16, (height + 15) / 16, 1);

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = bloomImages[i + 1];
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

    auto* hblurPipeline = pipelineManager.GetBloomHorizontalBlurPipeline("bloom_horizontal");
    auto* vblurPipeline = pipelineManager.GetBloomVerticalBlurPipeline("bloom_vertical");

    for (uint32_t i = bloomMipLevels - 1; i >= bloomMipLevels - 3 && i < bloomMipLevels; i--) {

        uint32_t w = std::max(extent.width >> i, 1u);
        uint32_t h = std::max(extent.height >> i, 1u);

        BloomBlurUniforms u{};
        u.inputResolution = glm::vec2(w, h);
        u.outputResolution = glm::vec2(w, h);
        u.texelSize = glm::vec2(1.0f / w, 1.0f / h);
        u.blurRadius = 1.0f;
        u.blurSamples = 8;

        UpdateUniform(vulkan.device, bloomUniformBufferMemories[i], u);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, hblurPipeline->pipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, bloomPipelineLayout, 0, 1, &bloomDescriptorSets[i * 2], 0, nullptr);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, bloomPipelineLayout, 1, 1, &bloomUniformDescriptorSets[i], 0, nullptr);

        uint32_t width = std::max(extent.width >> i, 1u);
        uint32_t height = std::max(extent.height >> i, 1u);
        vkCmdDispatch(commandBuffer, (width + 15) / 16, (height + 15) / 16, 1);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vblurPipeline->pipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, bloomPipelineLayout, 0, 1, &bloomDescriptorSets[i * 2], 0, nullptr);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, bloomPipelineLayout, 1, 1, &bloomUniformDescriptorSets[i], 0, nullptr);

        vkCmdDispatch(commandBuffer, (width + 15) / 16, (height + 15) / 16, 1);
    }

    auto* upsamplePipeline = pipelineManager.GetBloomUpsamplePipeline("bloom_upsample");
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, upsamplePipeline->pipeline);

    for (int32_t i = bloomMipLevels - 2; i >= 0; i--) {

        uint32_t srcW = std::max(extent.width >> (i + 1), 1u);
        uint32_t srcH = std::max(extent.height >> (i + 1), 1u);
        uint32_t dstW = std::max(extent.width >> i, 1u);
        uint32_t dstH = std::max(extent.height >> i, 1u);

        BloomUpsampleUniforms u{};
        u.bloomResolution = glm::vec2(srcW, srcH);
        u.outputResolution = glm::vec2(dstW, dstH);
        u.bloomTexelSize = glm::vec2(1.0f / (float)srcW, 1.0f / (float)srcH);
        u.filterRadius = 1.0f;

        UpdateUniform(vulkan.device, bloomUniformBufferMemories[i], u);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, bloomPipelineLayout, 0, 1, &bloomDescriptorSets[i * 2], 0, nullptr);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, bloomPipelineLayout, 1, 1, &bloomUniformDescriptorSets[i], 0, nullptr);

        uint32_t width = std::max(extent.width >> i, 1u);
        uint32_t height = std::max(extent.height >> i, 1u);
        vkCmdDispatch(commandBuffer, (width + 15) / 16, (height + 15) / 16, 1);
    }

    VkImageMemoryBarrier hdrToSample{};
    hdrToSample.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    hdrToSample.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    hdrToSample.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    hdrToSample.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    hdrToSample.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    hdrToSample.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    hdrToSample.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    hdrToSample.image = hdrColorImage;
    hdrToSample.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    hdrToSample.subresourceRange.baseMipLevel = 0;
    hdrToSample.subresourceRange.levelCount = 1;
    hdrToSample.subresourceRange.baseArrayLayer = 0;
    hdrToSample.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &hdrToSample
    );
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
    VkDevice device = vulkan.device;
    VkPhysicalDevice physicalDevice = vulkan.physicalDevice;

    VkExtent2D extent = vulkan.swapchainExtent;
    bloomMipLevels = static_cast<uint32_t>(std::floor(log2(std::max(extent.width, extent.height)))) + 1;

    VkFormat bloomFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

    for (uint32_t i = 0; i < bloomMipLevels; i++) {
        VkExtent3D imageExtent;
        imageExtent.width = std::max(extent.width >> i, 1u);
        imageExtent.height = std::max(extent.height >> i, 1u);
        imageExtent.depth = 1;

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = bloomFormat;
        imageInfo.extent = imageExtent;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                         VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VkImage image;
        if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create bloom image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;

        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t j = 0; j < memProperties.memoryTypeCount; j++) {
            if ((memRequirements.memoryTypeBits & (1 << j)) &&
                (memProperties.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
                allocInfo.memoryTypeIndex = j;
                break;
            }
        }

        VkDeviceMemory memory;
        if (vkAllocateMemory(device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate bloom image memory!");
        }

        if (vkBindImageMemory(device, image, memory, 0) != VK_SUCCESS) {
            throw std::runtime_error("Failed to bind bloom image memory!");
        }

        bloomImages.push_back(image);
        bloomImageMemories.push_back(memory);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = bloomFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create bloom image view!");
        }

        bloomImageViews.push_back(imageView);
    }

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.anisotropyEnable = VK_FALSE;

    vkCreateSampler(device, &samplerInfo, nullptr, &bloomSampler);

    {
        std::vector<VkDescriptorPoolSize> poolSizes;
        poolSizes.push_back({VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, bloomMipLevels});
        poolSizes.push_back({VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, bloomMipLevels});
        poolSizes.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, bloomMipLevels});

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = bloomMipLevels * 3;

        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &bloomDescriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create bloom descriptor pool!");
        }
    }

    {
        std::vector<VkDescriptorSetLayout> layouts(bloomMipLevels * 2, bloomDescriptorSetLayout);

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = bloomDescriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
        allocInfo.pSetLayouts = layouts.data();

        bloomDescriptorSets.resize(bloomMipLevels * 2);
        if (vkAllocateDescriptorSets(device, &allocInfo, bloomDescriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate bloom descriptor sets!");
        }

        std::vector<VkWriteDescriptorSet> writes(bloomMipLevels * 2);
        for (uint32_t i = 0; i < bloomMipLevels; i++) {

            writes[i * 2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[i * 2].dstSet = bloomDescriptorSets[i * 2];
            writes[i * 2].dstBinding = 0;
            writes[i * 2].dstArrayElement = 0;
            writes[i * 2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[i * 2].descriptorCount = 1;

            VkImageViewCreateInfo imageViewInfo{};
            imageViewInfo.image = bloomImages[i];
            VkImageView inputImageView = bloomImageViews[i];

            VkSampler sampler;
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
            vkCreateSampler(device, &samplerInfo, nullptr, &sampler);

            VkDescriptorImageInfo imageInfo{};
            imageInfo.sampler = sampler;
            imageInfo.imageView = inputImageView;
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

            writes[i * 2].pImageInfo = &imageInfo;

            writes[i * 2].dstBinding = 1;
            writes[i * 2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            writes[i * 2].descriptorCount = 1;

            VkDescriptorImageInfo storageImageInfo{};
            storageImageInfo.imageView = bloomImageViews[i];
            storageImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

            writes[i * 2].pImageInfo = &storageImageInfo;
        }

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }
}

void Renderer::CleanupBloomImages(VulkanResources& vulkan) {
    vkDeviceWaitIdle(vulkan.device);

    for (auto& imageView : bloomImageViews) {
        vkDestroyImageView(vulkan.device, imageView, nullptr);
    }
    for (auto& image : bloomImages) {
        vkDestroyImage(vulkan.device, image, nullptr);
    }
    for (auto& memory : bloomImageMemories) {
        vkFreeMemory(vulkan.device, memory, nullptr);
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
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

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

    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
        imageInfo.extent = {extent.width, extent.height, 1};
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        if (vkCreateImage(device, &imageInfo, nullptr, &hdrColorImage) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create HDR color image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, hdrColorImage, &memRequirements);

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

        if (vkAllocateMemory(device, &allocInfo, nullptr, &hdrColorImageMemory) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate HDR color image memory!");
        }

        if (vkBindImageMemory(device, hdrColorImage, hdrColorImageMemory, 0) != VK_SUCCESS) {
            throw std::runtime_error("Failed to bind HDR color image memory!");
        }

        VkCommandBuffer commandBuffer = Meridian::Internal::BeginSingleTimeCommand(vulkan.device, vulkan.commandPool);

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.image = hdrColorImage;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        VkClearColorValue clearColor{{0.0f, 0.0f, 0.0f, 1.0f}};
        VkImageSubresourceRange range{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        vkCmdClearColorImage(commandBuffer, hdrColorImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &range);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        vkEndCommandBuffer(commandBuffer);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = hdrColorImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &viewInfo, nullptr, &hdrColorImageView) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create HDR color image view!");
        }
    }

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

    {
        std::vector<VkImageView> attachments = {hdrColorImageView, hdrDepthImageView};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = hdrRenderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = extent.width;
        framebufferInfo.height = extent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &hdrFramebuffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create HDR framebuffer!");
        }
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
}

void Renderer::RenderSceneToHDR(VulkanResources& vulkan, VkCommandBuffer commandBuffer) {
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = hdrRenderPass;
    renderPassInfo.framebuffer = hdrFramebuffer;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = vulkan.swapchainExtent;

    std::vector<VkClearValue> clearValues(2);
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    auto* standardPipeline = pipelineManager.GetStandardPipeline("standard");
    standardPipeline->Bind(commandBuffer);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = static_cast<float>(vulkan.swapchainExtent.height);
    viewport.width = static_cast<float>(vulkan.swapchainExtent.width);
    viewport.height = -static_cast<float>(vulkan.swapchainExtent.height);
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
        0,
        1,
        &vulkan.descriptorSets[vulkan.currentFrame],
        0,
        nullptr
    );

    VkBuffer vertexBuffers[] = {testAsset.GetVertexBuffer().GetBuffer()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    testAsset.GetIndexBuffer().Bind(commandBuffer);

    for (int i = 0; i < 1000; i++) {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(i * 5, 0.0f, 0.0f));
        PushConstants pushConstants{};
        pushConstants.model = model;

        vkCmdPushConstants(commandBuffer, standardPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &pushConstants);
        vkCmdDrawIndexed(commandBuffer, testAsset.GetIndexBuffer().GetIndexCount(), 1, 0, 0, 0);
    }

    vkCmdEndRenderPass(commandBuffer);
}

void Renderer::RenderComposite(VulkanResources& vulkan, VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = vulkan.renderPass;
    renderPassInfo.framebuffer = vulkan.framebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = vulkan.swapchainExtent;

    VkClearValue clearValue{};
    clearValue.color = {0.0f, 0.0f, 1.0f, 1.0f};

    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearValue;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    auto* compositePipeline = pipelineManager.GetCompositePipeline("composite");
    compositePipeline->Bind(commandBuffer);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.height = static_cast<float>(vulkan.swapchainExtent.height);
    viewport.width = static_cast<float>(vulkan.swapchainExtent.width);
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
        0,
        1,
        &compositeDescriptorSet,
        0,
        nullptr
    );

    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);
}

void Renderer::CleanupHDRResources(VulkanResources& vulkan) {
    vkDeviceWaitIdle(vulkan.device);

    if (hdrFramebuffer != VK_NULL_HANDLE) {
        vkDestroyFramebuffer(vulkan.device, hdrFramebuffer, nullptr);
        hdrFramebuffer = VK_NULL_HANDLE;
    }
    if (hdrRenderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(vulkan.device, hdrRenderPass, nullptr);
        hdrRenderPass = VK_NULL_HANDLE;
    }
    if (hdrColorImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(vulkan.device, hdrColorImageView, nullptr);
        hdrColorImageView = VK_NULL_HANDLE;
    }
    if (hdrColorImage != VK_NULL_HANDLE) {
        vkDestroyImage(vulkan.device, hdrColorImage, nullptr);
        hdrColorImage = VK_NULL_HANDLE;
    }
    if (hdrColorImageMemory != VK_NULL_HANDLE) {
        vkFreeMemory(vulkan.device, hdrColorImageMemory, nullptr);
        hdrColorImageMemory = VK_NULL_HANDLE;
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
    VkDevice device = vulkan.device;
    VkPhysicalDevice physicalDevice = vulkan.physicalDevice;

    VkPhysicalDeviceMemoryProperties memProperties{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    bloomUniformBuffers.resize(bloomMipLevels);
    bloomUniformBufferMemories.resize(bloomMipLevels);
    bloomUniformDescriptorSets.resize(bloomMipLevels);

    for (uint32_t i = 0; i < bloomMipLevels; i++) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = sizeof(BloomDownsampleUniforms);
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &bufferInfo, nullptr, &bloomUniformBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create bloom uniform buffer!");
        }

        VkMemoryRequirements memRequirements{};
        vkGetBufferMemoryRequirements(device, bloomUniformBuffers[i], &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = Meridian::Internal::FindMemoryType(
            physicalDevice,
            memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        if (vkAllocateMemory(device, &allocInfo, nullptr, &bloomUniformBufferMemories[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate bloom uniform buffer memory!");
        }

        if (vkBindBufferMemory(device, bloomUniformBuffers[i], bloomUniformBufferMemories[i], 0) != VK_SUCCESS) {
            throw std::runtime_error("Failed to bind bloom uniform buffer memory!");
        }
    }

    std::vector<VkDescriptorSetLayout> layouts(bloomMipLevels, bloomDescriptorSetLayout2);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = bloomDescriptorPool;
    allocInfo.descriptorSetCount = bloomMipLevels;
    allocInfo.pSetLayouts = layouts.data();

    VkResult result = vkAllocateDescriptorSets(device, &allocInfo, bloomUniformDescriptorSets.data());
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate bloom uniform descriptor sets!");
    }

    std::vector<VkWriteDescriptorSet> writes(bloomMipLevels);
    std::vector<VkDescriptorBufferInfo> bufferInfos(bloomMipLevels);

    for (uint32_t i = 0; i < bloomMipLevels; i++) {
        bufferInfos[i].buffer = bloomUniformBuffers[i];
        bufferInfos[i].offset = 0;
        bufferInfos[i].range = sizeof(BloomDownsampleUniforms);

        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = bloomUniformDescriptorSets[i];
        writes[i].dstBinding = 0;
        writes[i].dstArrayElement = 0;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[i].descriptorCount = 1;
        writes[i].pBufferInfo = &bufferInfos[i];
    }

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

void Renderer::UpdateBloomUniformBuffers(VulkanResources& vulkan) {
    VkExtent2D extent = vulkan.swapchainExtent;

    for (uint32_t i = 0; i < bloomMipLevels; i++) {
        BloomDownsampleUniforms uniforms;
        uniforms.inputResolution = glm::vec2(
            static_cast<float>(std::max(extent.width >> i, 1u)),
            static_cast<float>(std::max(extent.height >> i, 1u))
        );
        uniforms.outputResolution = glm::vec2(
            static_cast<float>(std::max(extent.width >> (i + 1), 1u)),
            static_cast<float>(std::max(extent.height >> (i + 1), 1u))
        );
        uniforms.inputTexelSize = 1.0f / uniforms.inputResolution;

        void* data;
        vkMapMemory(vulkan.device, bloomUniformBufferMemories[i], 0, sizeof(BloomDownsampleUniforms), 0, &data);
        memcpy(data, &uniforms, sizeof(BloomDownsampleUniforms));
        vkUnmapMemory(vulkan.device, bloomUniformBufferMemories[i]);
    }
}

void Renderer::CreateCompositeDescriptorSet(VulkanResources& vulkan) {
    VkDescriptorSetLayout layouts[] = {
        compositeDescriptorSetLayout
    };

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = compositeDescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = layouts;

    vkAllocateDescriptorSets(
        vulkan.device,
        &allocInfo,
        &compositeDescriptorSet
    );

    VkDescriptorImageInfo imageInfo{};
    imageInfo.sampler = hdrSampler;
    imageInfo.imageView = hdrColorImageView;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkDescriptorImageInfo bloomInfo{};
    bloomInfo.sampler = bloomSampler;
    bloomInfo.imageView = bloomImageViews[0];
    bloomInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    VkWriteDescriptorSet writes[2]{};

    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = compositeDescriptorSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[0].descriptorCount = 1;
    writes[0].pImageInfo = &imageInfo;

    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = compositeDescriptorSet;
    writes[1].dstBinding = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].descriptorCount = 1;
    writes[1].pImageInfo = &bloomInfo;

    vkUpdateDescriptorSets(vulkan.device, 2, writes, 0, nullptr);
}