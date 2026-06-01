//
// Created by Dmitri Wamback on 2026-05-31.
//

#ifndef MERIDIAN_BLOOMUPSAMPLEPIPELINE_H
#define MERIDIAN_BLOOMUPSAMPLEPIPELINE_H

#include "../../Pipeline.h"
#include <string>

class BloomUpsamplePipeline : public Pipeline {
public:
    void Create(
        VkDevice device,
        VkPipelineLayout pipelineLayout,
        const std::string& computeShaderPath
    );

    void CreateDefaultDebug(VkDevice device, VkPipelineLayout pipelineLayout);

    ~BloomUpsamplePipeline();
};

#endif // MERIDIAN_BLOOMUPSAMPLEPIPELINE_H