//
// Created by Dmitri Wamback on 2026-05-30.
//

#include "PipelineManager.h"
#include <stdexcept>
#include <iostream>

PipelineManager::~PipelineManager() {
}

void PipelineManager::CreatePipeline(
    const std::string& name,
    VkDevice device,
    VkRenderPass renderPass,
    VkPipelineLayout pipelineLayout,
    const std::string& vertexShaderPath,
    const std::string& fragmentShaderPath
) {
    if (pipelines.find(name) != pipelines.end()) {
        throw std::runtime_error("Pipeline with name '" + name + "' already exists!");
    }

    this->device = device;

    auto pipeline = std::make_unique<Pipeline>();
    pipeline->Create(device, renderPass, pipelineLayout, vertexShaderPath, fragmentShaderPath);
    pipelines[name] = std::move(pipeline);
}

void PipelineManager::CreateStandardPipeline(
    const std::string& name,
    VkDevice device,
    VkRenderPass renderPass,
    VkPipelineLayout pipelineLayout,
    const std::string& vertexShaderPath,
    const std::string& fragmentShaderPath
) {
    if (standardPipelines.find(name) != standardPipelines.end()) {
        throw std::runtime_error("Standard pipeline with name '" + name + "' already exists!");
    }

    this->device = device;

    auto pipeline = std::make_unique<StandardPipeline>();
    pipeline->Create(device, renderPass, pipelineLayout, vertexShaderPath, fragmentShaderPath);
    standardPipelines[name] = std::move(pipeline);
}

void PipelineManager::CreateStandardPipelineDebug(
    const std::string& name,
    VkDevice device,
    VkRenderPass renderPass,
    VkPipelineLayout pipelineLayout
) {
    if (standardPipelines.find(name) != standardPipelines.end()) {
        throw std::runtime_error("Standard pipeline with name '" + name + "' already exists!");
    }

    this->device = device;

    auto pipeline = std::make_unique<StandardPipeline>();
    pipeline->CreateDefaultDebug(device, renderPass, pipelineLayout);
    standardPipelines[name] = std::move(pipeline);
}

void PipelineManager::CreateGBufferPipeline(
    const std::string& name,
    VkDevice device,
    VkRenderPass gbufferRenderPass,
    VkPipelineLayout pipelineLayout,
    const std::string& vertexShaderPath,
    const std::string& fragmentShaderPath
) {
    if (gbufferPipelines.find(name) != gbufferPipelines.end()) {
        throw std::runtime_error("G-buffer pipeline with name '" + name + "' already exists!");
    }

    this->device = device;

    auto pipeline = std::make_unique<GBufferPipeline>();
    pipeline->Create(device, gbufferRenderPass, pipelineLayout, vertexShaderPath, fragmentShaderPath);
    gbufferPipelines[name] = std::move(pipeline);
}

Pipeline* PipelineManager::GetPipeline(const std::string& name) {
    auto it = pipelines.find(name);
    if (it == pipelines.end()) {
        throw std::runtime_error("Pipeline '" + name + "' not found!");
    }
    return it->second.get();
}

const Pipeline* PipelineManager::GetPipeline(const std::string& name) const {
    auto it = pipelines.find(name);
    if (it == pipelines.end()) {
        throw std::runtime_error("Pipeline '" + name + "' not found!");
    }
    return it->second.get();
}

StandardPipeline* PipelineManager::GetStandardPipeline(const std::string& name) {
    auto it = standardPipelines.find(name);
    if (it == standardPipelines.end()) {
        throw std::runtime_error("Standard pipeline '" + name + "' not found!");
    }
    return it->second.get();
}

const StandardPipeline* PipelineManager::GetStandardPipeline(const std::string& name) const {
    auto it = standardPipelines.find(name);
    if (it == standardPipelines.end()) {
        throw std::runtime_error("Standard pipeline '" + name + "' not found!");
    }
    return it->second.get();
}

GBufferPipeline* PipelineManager::GetGBufferPipeline(const std::string& name) {
    auto it = gbufferPipelines.find(name);
    if (it == gbufferPipelines.end()) {
        throw std::runtime_error("G-buffer pipeline '" + name + "' not found!");
    }
    return it->second.get();
}

const GBufferPipeline* PipelineManager::GetGBufferPipeline(const std::string& name) const {
    auto it = gbufferPipelines.find(name);
    if (it == gbufferPipelines.end()) {
        throw std::runtime_error("G-buffer pipeline '" + name + "' not found!");
    }
    return it->second.get();
}

void PipelineManager::CreateBloomDownsamplePipeline(
    const std::string& name,
    VkDevice device,
    VkPipelineLayout pipelineLayout,
    const std::string& computeShaderPath
) {
    auto pipeline = std::make_unique<BloomDownsamplePipeline>();
    pipeline->Create(device, pipelineLayout, computeShaderPath);
    bloomDownsamplePipelines[name] = std::move(pipeline);
    this->device = device;
}

void PipelineManager::CreateBloomUpsamplePipeline(
    const std::string& name,
    VkDevice device,
    VkPipelineLayout pipelineLayout,
    const std::string& computeShaderPath
) {
    auto pipeline = std::make_unique<BloomUpsamplePipeline>();
    pipeline->Create(device, pipelineLayout, computeShaderPath);
    bloomUpsamplePipelines[name] = std::move(pipeline);
    this->device = device;
}

void PipelineManager::CreateCompositePipeline(
    const std::string& name,
    VkDevice device,
    VkRenderPass renderPass,
    VkPipelineLayout pipelineLayout,
    const std::string& vertexShaderPath,
    const std::string& fragmentShaderPath
) {
    if (compositePipelines.find(name) != compositePipelines.end()) {
        throw std::runtime_error("Composite pipeline with name '" + name + "' already exists!");
    }

    this->device = device;

    auto pipeline = std::make_unique<CompositePipeline>();
    pipeline->Create(device, renderPass, pipelineLayout, vertexShaderPath, fragmentShaderPath);
    compositePipelines[name] = std::move(pipeline);
}

CompositePipeline* PipelineManager::GetCompositePipeline(const std::string& name) {
    auto it = compositePipelines.find(name);
    if (it == compositePipelines.end()) {
        throw std::runtime_error("Composite pipeline '" + name + "' not found!");
    }
    return it->second.get();
}

BloomDownsamplePipeline* PipelineManager::GetBloomDownsamplePipeline(const std::string& name) {
    auto it = bloomDownsamplePipelines.find(name);
    return (it != bloomDownsamplePipelines.end()) ? it->second.get() : nullptr;
}

BloomUpsamplePipeline* PipelineManager::GetBloomUpsamplePipeline(const std::string& name) {
    auto it = bloomUpsamplePipelines.find(name);
    return (it != bloomUpsamplePipelines.end()) ? it->second.get() : nullptr;
}

bool PipelineManager::HasPipeline(const std::string& name) const {
    return pipelines.find(name) != pipelines.end() ||
           standardPipelines.find(name) != standardPipelines.end() ||
           gbufferPipelines.find(name) != gbufferPipelines.end();
}

void PipelineManager::Cleanup(VkDevice device) {
    pipelines.clear();
    standardPipelines.clear();
    gbufferPipelines.clear();
}