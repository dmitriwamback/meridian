//
// Created by Dmitri Wamback on 2026-05-30.
//

#ifndef MERIDIAN_PIPELINEMANAGER_H
#define MERIDIAN_PIPELINEMANAGER_H

#include "Pipeline.h"
#include "types/StandardPipeline.h"
#include "types/GBufferPipeline.h"
#include "types/bloom/BloomDownsamplePipeline.h"
#include "types/bloom/BloomUpsamplePipeline.h"
#include "types/CompositePipeline.h"

#include <unordered_map>
#include <string>
#include <vector>
#include <memory>

class PipelineManager {
public:
    PipelineManager() = default;
    ~PipelineManager();

    void CreatePipeline(
        const std::string& name,
        VkDevice device,
        VkRenderPass renderPass,
        VkPipelineLayout pipelineLayout,
        const std::string& vertexShaderPath,
        const std::string& fragmentShaderPath
    );

    void CreateStandardPipeline(
        const std::string& name,
        VkDevice device,
        VkRenderPass renderPass,
        VkPipelineLayout pipelineLayout,
        const std::string& vertexShaderPath,
        const std::string& fragmentShaderPath
    );

    void CreateStandardPipelineDebug(
        const std::string& name,
        VkDevice device,
        VkRenderPass renderPass,
        VkPipelineLayout pipelineLayout
    );

    void CreateGBufferPipeline(
        const std::string& name,
        VkDevice device,
        VkRenderPass gbufferRenderPass,
        VkPipelineLayout pipelineLayout,
        const std::string& vertexShaderPath,
        const std::string& fragmentShaderPath
    );

    void CreateBloomDownsamplePipeline(
        const std::string& name,
        VkDevice device,
        VkPipelineLayout pipelineLayout,
        const std::string& computeShaderPath
    );

    void CreateBloomUpsamplePipeline(
        const std::string& name,
        VkDevice device,
        VkPipelineLayout pipelineLayout,
        const std::string& computeShaderPath
    );

    void CreateBloomHorizontalBlurPipeline(
        const std::string& name,
        VkDevice device,
        VkPipelineLayout pipelineLayout,
        const std::string& computeShaderPath
    );

    void CreateBloomVerticalBlurPipeline(
        const std::string& name,
        VkDevice device,
        VkPipelineLayout pipelineLayout,
        const std::string& computeShaderPath
    );

    void CreateCompositePipeline(
        const std::string& name,
        VkDevice device,
        VkRenderPass gbufferRenderPass,
        VkPipelineLayout pipelineLayout,
        const std::string& vertexShaderPath,
        const std::string& fragmentShaderPath
    );

    Pipeline* GetPipeline(const std::string& name);
    const Pipeline* GetPipeline(const std::string& name) const;

    StandardPipeline* GetStandardPipeline(const std::string& name);
    const StandardPipeline* GetStandardPipeline(const std::string& name) const;

    GBufferPipeline* GetGBufferPipeline(const std::string& name);
    const GBufferPipeline* GetGBufferPipeline(const std::string& name) const;

    BloomDownsamplePipeline* GetBloomDownsamplePipeline(const std::string& name);
    BloomUpsamplePipeline* GetBloomUpsamplePipeline(const std::string& name);

    CompositePipeline* GetCompositePipeline(const std::string& name);

    bool HasPipeline(const std::string& name) const;

    void Cleanup(VkDevice device);

private:
    std::unordered_map<std::string, std::unique_ptr<Pipeline>> pipelines;
    std::unordered_map<std::string, std::unique_ptr<StandardPipeline>> standardPipelines;
    std::unordered_map<std::string, std::unique_ptr<GBufferPipeline>> gbufferPipelines;
    std::unordered_map<std::string, std::unique_ptr<BloomDownsamplePipeline>> bloomDownsamplePipelines;
    std::unordered_map<std::string, std::unique_ptr<BloomUpsamplePipeline>> bloomUpsamplePipelines;
    std::unordered_map<std::string, std::unique_ptr<CompositePipeline>> compositePipelines;
    VkDevice device = VK_NULL_HANDLE;
};

#endif // MERIDIAN_PIPELINEMANAGER_H