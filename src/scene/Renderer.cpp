//
// Created by Dmitri Wamback on 2026-05-30.
//

#include "Renderer.h"
#include <stdexcept>
#include <iostream>

#include "../memory/Internal.h"

Renderer::~Renderer() {
}

void Renderer::Initialize(GLFWwindow* window, VulkanResources& vulkan) {
    if (initialized) {
        throw std::runtime_error("Renderer already initialized!");
    }

    CreatePipelineLayouts(vulkan);
    CreateDescriptorSets(vulkan);
    SetupCommandBuffers(vulkan);

    pipelineManager.CreateStandardPipeline(
        "standard",
        vulkan.device,
        vulkan.renderPass,
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
    // Standard pipeline layout (UBO + texture sampler)

    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(vulkan.physicalDevice, &memoryProperties);

    uniformBuffer = std::make_unique<UniformBuffer>(
        vulkan.device,
        memoryProperties,
        sizeof(UniformBufferObject)
    );

    uniformBufferObject.model = glm::mat4();
    uniformBufferObject.view = glm::mat4();
    uniformBufferObject.proj = glm::mat4();

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

    {
        std::vector<VkDescriptorSetLayout> setLayouts(1);
        setLayouts[0] = vulkan.descriptorSetLayout;

        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
        layoutInfo.pSetLayouts = setLayouts.data();

        if (vkCreatePipelineLayout(vulkan.device, &layoutInfo, nullptr, &standardPipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create standard pipeline layout!");
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
}

void Renderer::Render(VulkanResources& vulkan) {
    if (!initialized) {
        throw std::runtime_error("Renderer not initialized!");
    }

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

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = vulkan.renderPass;
    renderPassInfo.framebuffer = vulkan.framebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = vulkan.swapchainExtent;

    std::vector<VkClearValue> clearValues(2);
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};  // Black
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(vulkan.commandBuffers[vulkan.currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    auto* standardPipeline = pipelineManager.GetStandardPipeline("standard");
    standardPipeline->Bind(vulkan.commandBuffers[vulkan.currentFrame]);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(vulkan.swapchainExtent.width);
    viewport.height = static_cast<float>(vulkan.swapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(vulkan.commandBuffers[vulkan.currentFrame], 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = vulkan.swapchainExtent;
    vkCmdSetScissor(vulkan.commandBuffers[vulkan.currentFrame], 0, 1, &scissor);

    vkCmdBindDescriptorSets(
        vulkan.commandBuffers[vulkan.currentFrame],
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        standardPipelineLayout,
        0,
        1,
        &vulkan.descriptorSets[vulkan.currentFrame],
        0,
        nullptr
    );

    vkCmdEndRenderPass(vulkan.commandBuffers[vulkan.currentFrame]);

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
        vulkan.framebuffers
    );
}

void Renderer::Cleanup(VulkanResources& vulkan) {
    if (!initialized) return;

    vkDeviceWaitIdle(vulkan.device);

    pipelineManager.Cleanup(vulkan.device);

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