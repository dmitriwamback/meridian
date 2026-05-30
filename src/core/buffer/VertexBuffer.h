//
// Created by Dmitri Wamback on 2026-05-30.
//

#ifndef MERIDIAN_VERTEXBUFFER_H
#define MERIDIAN_VERTEXBUFFER_H

#include <vulkan/vulkan.h>
#include <vector>
#include "Vertex.h"

class VertexBuffer {
public:
    VertexBuffer() = default;
    ~VertexBuffer();

    void Create(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        const std::vector<Vertex>& vertices
    );

    void Bind(VkCommandBuffer commandBuffer, uint32_t binding = 0) const;

    VkBuffer GetBuffer() const;

    void Cleanup(VkDevice device);

private:
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory bufferMemory = VK_NULL_HANDLE;
    size_t size = 0;
};

#endif // MERIDIAN_VERTEXBUFFER_H