//
// Created by Dmitri Wamback on 2026-05-30.
//

#ifndef MERIDIAN_ASSET_H
#define MERIDIAN_ASSET_H

#include <string>
#include <vector>
#include <memory>
#include <vulkan/vulkan.h>

#include "../buffer/IndexBuffer.h"
#include "../buffer/VertexBuffer.h"

class Asset {
public:
    Asset() = default;
    ~Asset();

    void Create();

    const std::string& GetName() const;
    void SetName(const std::string& name);

    const std::vector<Vertex>& GetVertices() const;

    const std::vector<uint32_t>& GetIndices() const;

    void CreateBuffers(VkDevice device, VkPhysicalDevice physicalDevice);

    void Cleanup(VkDevice device);

    bool IsLoaded() const;

    VertexBuffer GetVertexBuffer() const;
    IndexBuffer GetIndexBuffer() const;

private:
    std::string name;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    VertexBuffer vertexBuffer;
    IndexBuffer indexBuffer;

    bool loaded = false;
};

#endif // MERIDIAN_ASSET_H