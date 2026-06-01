//
// Created by Dmitri Wamback on 2026-05-31.
//

#include "BloomVerticalBlurPipeline.h"
#include <stdexcept>

BloomVerticalBlurPipeline::~BloomVerticalBlurPipeline() {
}

void BloomVerticalBlurPipeline::Create(
    VkDevice device,
    VkPipelineLayout pipelineLayout,
    const std::string& computeShaderPath
) {
    this->pipelineLayout = pipelineLayout;

    VkShaderModule computeShaderModule = LoadShaderModule(device, computeShaderPath);

    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stageInfo.module = computeShaderModule;
    stageInfo.pName = "main";

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage = stageInfo;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
        vkDestroyShaderModule(device, computeShaderModule, nullptr);
        throw std::runtime_error("Failed to create bloom vertical blur compute pipeline!");
    }

    vkDestroyShaderModule(device, computeShaderModule, nullptr);
}

void BloomVerticalBlurPipeline::CreateDefaultDebug(
    VkDevice device,
    VkPipelineLayout pipelineLayout
) {
    Create(device, pipelineLayout, "/Users/dmitriwamback/CLionProjects/meridian/shaders/bloom_vertical_blur.spv");
}