//
// Created by Dmitri Wamback on 2026-05-30.
//

#ifndef MERIDIAN_PIPELINE_H
#define MERIDIAN_PIPELINE_H

#include <vulkan/vulkan.h>
#include <string>
#include <vector>

class Pipeline {
public:
    Pipeline() = default;
    ~Pipeline();

    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

    // Create the graphics pipeline
    void Create(
        VkDevice device,
        VkRenderPass renderPass,
        VkPipelineLayout pipelineLayout,
        const std::string& vertexShaderPath,
        const std::string& fragmentShaderPath
    );

    // Bind the pipeline
    void Bind(VkCommandBuffer commandBuffer) const;

    // Get the pipeline handle
    VkPipeline GetHandle() const;

    // Cleanup
    void Cleanup(VkDevice device);

    // Load shader module from file
    VkShaderModule LoadShaderModule(VkDevice device, const std::string& filePath);

    // Read file into binary code
    static std::vector<char> ReadFile(const std::string& filePath);
};

#endif // MERIDIAN_PIPELINE_H