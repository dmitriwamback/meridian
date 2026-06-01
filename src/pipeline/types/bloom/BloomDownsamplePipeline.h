//
// Created by Dmitri Wamback on 2026-05-31.
//

#ifndef MERIDIAN_BLOOMDOWNSAMPLEPIPELINE_H
#define MERIDIAN_BLOOMDOWNSAMPLEPIPELINE_H

#include "../../Pipeline.h"
#include <string>

class BloomDownsamplePipeline : public Pipeline {
public:
    void Create(
        VkDevice device,
        VkPipelineLayout pipelineLayout,
        const std::string& computeShaderPath
    );

    void CreateDefaultDebug(VkDevice device, VkPipelineLayout pipelineLayout);

    ~BloomDownsamplePipeline();
};

#endif // MERIDIAN_BLOOMDOWNSAMPLEPIPELINE_H