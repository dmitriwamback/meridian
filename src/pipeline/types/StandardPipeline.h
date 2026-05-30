//
// Created by Dmitri Wamback on 2026-05-30.
//

#ifndef MERIDIAN_STANDARDPIPELINE_H
#define MERIDIAN_STANDARDPIPELINE_H

#include "../Pipeline.h"
#include <vulkan/vulkan.h>
#include <string>

class StandardPipeline : public Pipeline {
public:
    StandardPipeline() = default;
    ~StandardPipeline() = default;

    void Create(
        VkDevice device,
        VkRenderPass renderPass,
        VkPipelineLayout pipelineLayout,
        const std::string& vertexShaderPath,
        const std::string& fragmentShaderPath,
        uint32_t maxUniformBuffers = 2
    );

    void CreateDefaultDebug(
        VkDevice device,
        VkRenderPass renderPass,
        VkPipelineLayout pipelineLayout
    );

private:
    static std::vector<VkVertexInputBindingDescription> GetBindingDescriptions();
    static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions();
};

#endif // MERIDIAN_STANDARDPIPELINE_H