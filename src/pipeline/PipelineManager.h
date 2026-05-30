//
// Created by Dmitri Wamback on 2026-05-30.
//

#ifndef MERIDIAN_PIPELINEMANAGER_H
#define MERIDIAN_PIPELINEMANAGER_H

#include "Pipeline.h"
#include "types/StandardPipeline.h"
#include "types/GBufferPipeline.h"
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

    Pipeline* GetPipeline(const std::string& name);
    const Pipeline* GetPipeline(const std::string& name) const;

    StandardPipeline* GetStandardPipeline(const std::string& name);
    const StandardPipeline* GetStandardPipeline(const std::string& name) const;

    GBufferPipeline* GetGBufferPipeline(const std::string& name);
    const GBufferPipeline* GetGBufferPipeline(const std::string& name) const;

    bool HasPipeline(const std::string& name) const;

    void Cleanup(VkDevice device);

private:
    std::unordered_map<std::string, std::unique_ptr<Pipeline>> pipelines;
    std::unordered_map<std::string, std::unique_ptr<StandardPipeline>> standardPipelines;
    std::unordered_map<std::string, std::unique_ptr<GBufferPipeline>> gbufferPipelines;
    VkDevice device = VK_NULL_HANDLE;
};

#endif // MERIDIAN_PIPELINEMANAGER_H