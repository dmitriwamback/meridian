//
// Created by Dmitri Wamback on 2026-05-30.
//

#ifndef MERIDIAN_UNIFORMBUFFER_H
#define MERIDIAN_UNIFORMBUFFER_H

#include <vulkan/vulkan.h>
#include <glm/glm/glm.hpp>
#include <vector>

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

struct BloomDownsampleUniforms {
    glm::vec2 inputResolution;
    glm::vec2 outputResolution;
    glm::vec2 inputTexelSize;
};

struct BloomUpsampleUniforms {
    glm::vec2 bloomResolution;
    glm::vec2 outputResolution;
    glm::vec2 bloomTexelSize;
    float filterRadius;
    float padding[12];  // Pad to 128 bytes
};

struct BloomBlurUniforms {
    glm::vec2 inputResolution;
    glm::vec2 outputResolution;
    glm::vec2 texelSize;
    float blurRadius;
    int blurSamples;
    int padding[11];  // Pad to 128 bytes
};

class UniformBuffer {
public:
    UniformBuffer(VkDevice device, VkPhysicalDeviceMemoryProperties memoryProperties, size_t size);
    ~UniformBuffer();

    UniformBuffer(const UniformBuffer&) = delete;
    UniformBuffer& operator=(const UniformBuffer&) = delete;

    void Update(const void* data, size_t size);
    uint32_t FindMemoryType(VkPhysicalDeviceMemoryProperties memoryProperties, uint32_t typeFilter, VkMemoryPropertyFlags properties);

    VkBuffer GetBuffer() const { return buffer; }
    VkDeviceMemory GetMemory() const { return memory; }

private:
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkDevice device_;
};

template <typename T>
void UpdateUniform(VkDevice device, VkDeviceMemory memory, const T& ubo) {
    void* data = nullptr;
    vkMapMemory(device, memory, 0, sizeof(T), 0, &data);
    memcpy(data, &ubo, sizeof(T));
    vkUnmapMemory(device, memory);
}

#endif //MERIDIAN_UNIFORMBUFFER_H