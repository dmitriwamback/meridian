//
// Created by Dmitri Wamback on 2026-05-30.
//

#include "VertexBuffer.h"
#include "../../memory/Internal.h"
#include <stdexcept>

VertexBuffer::~VertexBuffer() {
    // Should be cleaned up explicitly with Cleanup()
}

void VertexBuffer::Create(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    const std::vector<Vertex>& vertices
) {
    size = vertices.size() * sizeof(Vertex);

    Meridian::Internal::CreateBuffer(
        device,
        physicalDevice,
        size,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        buffer,
        bufferMemory
    );

    void* data;
    vkMapMemory(device, bufferMemory, 0, size, 0, &data);
    std::memcpy(data, vertices.data(), size);
    vkUnmapMemory(device, bufferMemory);
}

void VertexBuffer::Bind(VkCommandBuffer commandBuffer, uint32_t binding) const {
    VkBuffer buffers[] = {buffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, binding, 1, buffers, offsets);
}

VkBuffer VertexBuffer::GetBuffer() const {
    return buffer;
}

void VertexBuffer::Cleanup(VkDevice device) {
    if (buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, buffer, nullptr);
        buffer = VK_NULL_HANDLE;
    }
    if (bufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, bufferMemory, nullptr);
        bufferMemory = VK_NULL_HANDLE;
    }
}