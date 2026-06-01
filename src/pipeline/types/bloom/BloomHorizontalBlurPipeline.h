//
// Created by Dmitri Wamback on 2026-05-31.
//

#ifndef MERIDIAN_BLOOMHORIZONTALBLURPIPELINE_H
#define MERIDIAN_BLOOMHORIZONTALBLURPIPELINE_H

#include "../../Pipeline.h"
#include <string>

class BloomHorizontalBlurPipeline : public Pipeline {
public:
    void Create(
        VkDevice device,
        VkPipelineLayout pipelineLayout,
        const std::string& computeShaderPath
    );

    void CreateDefaultDebug(VkDevice device, VkPipelineLayout pipelineLayout);

    ~BloomHorizontalBlurPipeline();
};

#endif // MERIDIAN_BLOOMHORIZONTALBLURPIPELINE_H