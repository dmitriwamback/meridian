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
#endif //MERIDIAN_UNIFORMBUFFER_H