//
// Created by Dmitri Wamback on 2026-05-30.
//

#include "Asset.h"
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <glm/glm/glm.hpp>

// Vertex binding description
VkVertexInputBindingDescription Vertex::GetBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescription;
}

// Vertex attribute descriptions
std::vector<VkVertexInputAttributeDescription> Vertex::GetAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions(3);

    // Position
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, position);

    // Normal
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, normal);

    // UV
    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex, uv);

    return attributeDescriptions;
}

Asset::~Asset() {
    // Should be cleaned up explicitly with Cleanup()
}

void Asset::Load(const std::string& filePath) {
    name = filePath;

    // TODO: Implement actual mesh loading (OBJ, FBX, etc.)
    // For now, create a simple cube as placeholder

    // Cube vertices (position, normal, uv)
    vertices = {
        // Front face
        {{-1.0f, -1.0f,  1.0f}, {0.0f, 0.0f,  1.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f,  1.0f}, {0.0f, 0.0f,  1.0f}, {1.0f, 0.0f}},
        {{ 1.0f,  1.0f,  1.0f}, {0.0f, 0.0f,  1.0f}, {1.0f, 1.0f}},
        {{-1.0f,  1.0f,  1.0f}, {0.0f, 0.0f,  1.0f}, {0.0f, 1.0f}},

        // Back face
        {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
        {{-1.0f,  1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
        {{ 1.0f,  1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
        {{ 1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},

        // Top face
        {{-1.0f,  1.0f, -1.0f}, {0.0f, 1.0f,  0.0f}, {0.0f, 0.0f}},
        {{-1.0f,  1.0f,  1.0f}, {0.0f, 1.0f,  0.0f}, {1.0f, 0.0f}},
        {{ 1.0f,  1.0f,  1.0f}, {0.0f, 1.0f,  0.0f}, {1.0f, 1.0f}},
        {{ 1.0f,  1.0f, -1.0f}, {0.0f, 1.0f,  0.0f}, {0.0f, 1.0f}},

        // Bottom face
        {{-1.0f, -1.0f, -1.0f}, {0.0f, -1.0f,  0.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f, -1.0f}, {0.0f, -1.0f,  0.0f}, {1.0f, 0.0f}},
        {{ 1.0f, -1.0f,  1.0f}, {0.0f, -1.0f,  0.0f}, {1.0f, 1.0f}},
        {{-1.0f, -1.0f,  1.0f}, {0.0f, -1.0f,  0.0f}, {0.0f, 1.0f}},

        // Right face
        {{ 1.0f, -1.0f, -1.0f}, {1.0f, 0.0f,  0.0f}, {0.0f, 0.0f}},
        {{ 1.0f,  1.0f, -1.0f}, {1.0f, 0.0f,  0.0f}, {1.0f, 0.0f}},
        {{ 1.0f,  1.0f,  1.0f}, {1.0f, 0.0f,  0.0f}, {1.0f, 1.0f}},
        {{ 1.0f, -1.0f,  1.0f}, {1.0f, 0.0f,  0.0f}, {0.0f, 1.0f}},

        // Left face
        {{-1.0f, -1.0f, -1.0f}, {-1.0f, 0.0f,  0.0f}, {0.0f, 0.0f}},
        {{-1.0f, -1.0f,  1.0f}, {-1.0f, 0.0f,  0.0f}, {1.0f, 0.0f}},
        {{-1.0f,  1.0f,  1.0f}, {-1.0f, 0.0f,  0.0f}, {1.0f, 1.0f}},
        {{-1.0f,  1.0f, -1.0f}, {-1.0f, 0.0f,  0.0f}, {0.0f, 1.0f}}
    };

    // Cube indices
    indices = {
        0, 1, 2,  2, 3, 0,  // Front
        4, 5, 6,  6, 7, 4,  // Back
        8, 9, 10, 10, 11, 8,  // Top
        12, 13, 14, 14, 15, 12,  // Bottom
        16, 17, 18, 18, 19, 16,  // Right
        20, 21, 22, 22, 23, 20  // Left
    };

    loaded = true;
    std::cout << "Asset loaded: " << filePath << " (" << vertices.size() << " vertices, "
              << indices.size() << " indices)" << std::endl;
}

const std::string& Asset::GetName() const {
    return name;
}

void Asset::SetName(const std::string& name) {
    this->name = name;
}

const std::vector<Vertex>& Asset::GetVertices() const {
    return vertices;
}

const std::vector<uint32_t>& Asset::GetIndices() const {
    return indices;
}

void Asset::CreateBuffers(VkDevice device, VkPhysicalDevice physicalDevice) {
    if (!loaded) {
        throw std::runtime_error("Asset not loaded!");
    }

    // Create vertex buffer
    Internal::CreateBuffer(
        device,
        physicalDevice,
        vertices.size() * sizeof(Vertex),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        vertexBuffer,
        vertexBufferMemory
    );

    // Copy vertices to buffer
    void* data;
    vkMapMemory(device, vertexBufferMemory, 0, vertices.size() * sizeof(Vertex), 0, &data);
    std::memcpy(data, vertices.data(), vertices.size() * sizeof(Vertex));
    vkUnmapMemory(device, vertexBufferMemory);

    // Create index buffer
    Internal::CreateBuffer(
        device,
        physicalDevice,
        indices.size() * sizeof(uint32_t),
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        indexBuffer,
        indexBufferMemory
    );

    // Copy indices to buffer
    vkMapMemory(device, indexBufferMemory, 0, indices.size() * sizeof(uint32_t), 0, &data);
    std::memcpy(data, indices.data(), indices.size() * sizeof(uint32_t));
    vkUnmapMemory(device, indexBufferMemory);
}

void Asset::Cleanup(VkDevice device) {
    if (vertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, vertexBuffer, nullptr);
        vertexBuffer = VK_NULL_HANDLE;
    }
    if (vertexBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, vertexBufferMemory, nullptr);
        vertexBufferMemory = VK_NULL_HANDLE;
    }
    if (indexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, indexBuffer, nullptr);
        indexBuffer = VK_NULL_HANDLE;
    }
    if (indexBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, indexBufferMemory, nullptr);
        indexBufferMemory = VK_NULL_HANDLE;
    }
}

bool Asset::IsLoaded() const {
    return loaded;
}