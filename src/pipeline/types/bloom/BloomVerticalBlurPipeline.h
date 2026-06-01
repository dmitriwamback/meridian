//
// Created by Dmitri Wamback on 2026-05-31.
//

#ifndef MERIDIAN_BLOOMVERTICALBLURPIPELINE_H
#define MERIDIAN_BLOOMVERTICALBLURPIPELINE_H

#include "../../Pipeline.h"
#include <string>

class BloomVerticalBlurPipeline : public Pipeline {
public:
    void Create(
        VkDevice device,
        VkPipelineLayout pipelineLayout,
        const std::string& computeShaderPath
    );

    void CreateDefaultDebug(VkDevice device, VkPipelineLayout pipelineLayout);

    ~BloomVerticalBlurPipeline();
};

#endif // MERIDIAN_BLOOMVERTICALBLURPIPELINE_H