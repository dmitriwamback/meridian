//
// Created by Dmitri Wamback on 2026-05-30.
//

#include "Asset.h"
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <glm/glm/glm.hpp>
#include "../buffer/Vertex.h"
#include "../buffer/VertexBuffer.h"
#include "../buffer/IndexBuffer.h"
#include "../../memory/Internal.h"

Asset::~Asset() {
    // Should be cleaned up explicitly with Cleanup()
}

void Asset::Create() {

    vertexBuffer = VertexBuffer();
    indexBuffer = IndexBuffer();

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

    vertexBuffer.Create(device, physicalDevice, vertices);
    indexBuffer.Create(device, physicalDevice, indices);
}

VertexBuffer Asset::GetVertexBuffer() const {
    return vertexBuffer;
}

IndexBuffer Asset::GetIndexBuffer() const {
    return indexBuffer;
}

void Asset::Cleanup(VkDevice device) {
    vertexBuffer.Cleanup(device);
    indexBuffer.Cleanup(device);
}

bool Asset::IsLoaded() const {
    return loaded;
}