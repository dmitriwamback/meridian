//
// Created by Dmitri Wamback on 2026-05-30.
//

#ifndef MERIDIAN_GBUFFERPIPELINE_H
#define MERIDIAN_GBUFFERPIPELINE_H

#include "../Pipeline.h"
#include <vulkan/vulkan.h>
#include <vector>

class GBufferPipeline : public Pipeline {
public:
    GBufferPipeline() = default;
    ~GBufferPipeline() = default;

    // Create the G-buffer pipeline with multiple attachments
    void Create(
        VkDevice device,
        VkRenderPass gbufferRenderPass,
        VkPipelineLayout pipelineLayout,
        const std::string& vertexShaderPath,
        const std::string& fragmentShaderPath
    );

    // Get the G-buffer render pass
    VkRenderPass GetRenderPass() const;

    // Get G-buffer attachments (images)
    const std::vector<VkImageView>& GetAttachments() const;

private:
    VkRenderPass gbufferRenderPass = VK_NULL_HANDLE;
    std::vector<VkImageView> gbufferAttachments;
};

#endif // MERIDIAN_GBUFFERPIPELINE_H