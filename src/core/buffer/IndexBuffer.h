//
// Created by Dmitri Wamback on 2026-05-30.
//

#ifndef MERIDIAN_INDEXBUFFER_H
#define MERIDIAN_INDEXBUFFER_H

#include <vulkan/vulkan.h>
#include <vector>

class IndexBuffer {
public:
    IndexBuffer() = default;
    ~IndexBuffer();

    void Create(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        const std::vector<uint32_t>& indices
    );

    void Bind(VkCommandBuffer commandBuffer) const;

    VkBuffer GetBuffer() const;

    uint32_t GetIndexCount() const;

    void Cleanup(VkDevice device);

private:
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory bufferMemory = VK_NULL_HANDLE;
    size_t size = 0;
    uint32_t indexCount = 0;
};

#endif //MERIDIAN_INDEXBUFFER_H