//
// Created by Dmitri Wamback on 2026-05-30.
//

#include "IndexBuffer.h"

#include "IndexBuffer.h"
#include "../../memory/Internal.h"
#include <stdexcept>

IndexBuffer::~IndexBuffer() {
    // Should be cleaned up explicitly with Cleanup()
}

void IndexBuffer::Create(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    const std::vector<uint32_t>& indices
) {
    size = indices.size() * sizeof(uint32_t);
    indexCount = static_cast<uint32_t>(indices.size());

    Meridian::Internal::CreateBuffer(
        device,
        physicalDevice,
        size,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        buffer,
        bufferMemory
    );

    void* data;
    vkMapMemory(device, bufferMemory, 0, size, 0, &data);
    std::memcpy(data, indices.data(), size);
    vkUnmapMemory(device, bufferMemory);
}

void IndexBuffer::Bind(VkCommandBuffer commandBuffer) const {
    vkCmdBindIndexBuffer(commandBuffer, buffer, 0, VK_INDEX_TYPE_UINT32);
}

VkBuffer IndexBuffer::GetBuffer() const {
    return buffer;
}

uint32_t IndexBuffer::GetIndexCount() const {
    return indexCount;
}

void IndexBuffer::Cleanup(VkDevice device) {
    if (buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, buffer, nullptr);
        buffer = VK_NULL_HANDLE;
    }
    if (bufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, bufferMemory, nullptr);
        bufferMemory = VK_NULL_HANDLE;
    }
}