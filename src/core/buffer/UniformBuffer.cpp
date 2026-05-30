//
// Created by Dmitri Wamback on 2026-05-30.
//

#include "UniformBuffer.h"
#include <stdexcept>

UniformBuffer::UniformBuffer(VkDevice device, VkPhysicalDeviceMemoryProperties memoryProperties, size_t size) : device_(device) {

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device_, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create uniform buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device_, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memoryProperties, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(device_, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate uniform buffer memory!");
    }

    if (vkBindBufferMemory(device_, buffer, memory, 0) != VK_SUCCESS) {
        throw std::runtime_error("Failed to bind uniform buffer memory!");
    }
}

UniformBuffer::~UniformBuffer() {
    vkDestroyBuffer(device_, buffer, nullptr);
    vkFreeMemory(device_, memory, nullptr);
}

void UniformBuffer::Update(const void* data, size_t size) {
    void* mapped;
    if (vkMapMemory(device_, memory, 0, size, 0, &mapped) != VK_SUCCESS) {
        throw std::runtime_error("Failed to map uniform buffer memory!");
    }

    std::memcpy(mapped, data, size);
    vkUnmapMemory(device_, memory);
}

uint32_t UniformBuffer::FindMemoryType(VkPhysicalDeviceMemoryProperties memoryProperties, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) &&
            (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type!");
}