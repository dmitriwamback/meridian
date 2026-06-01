//
// Created by Dmitri Wamback on 2026-05-31.
//

#ifndef MERIDIAN_COMPOSITEPIPELINE_H
#define MERIDIAN_COMPOSITEPIPELINE_H

#include <vulkan/vulkan.h>
#include "../Pipeline.h"

class CompositePipeline : public Pipeline {
public:
    CompositePipeline() = default;
    ~CompositePipeline() = default;

    void Create(
        VkDevice device,
        VkRenderPass renderPass,
        VkPipelineLayout pipelineLayout,
        const std::string& vertexShaderPath,
        const std::string& fragmentShaderPath
    );
};

#endif //MERIDIAN_COMPOSITEPIPELINE_H