//
// Created by Dmitri Wamback on 2026-05-30.
//

#ifndef MERIDIAN_ASSET_H
#define MERIDIAN_ASSET_H

#include <string>
#include <vector>
#include <memory>
#include <vulkan/vulkan.h>

class Asset {
public:
    Asset() = default;
    ~Asset();

    // Load asset from file (e.g., OBJ, FBX)
    void Load(const std::string& filePath);

    // Get asset name
    const std::string& GetName() const;
    void SetName(const std::string& name);

    // Get vertices
    const std::vector<Vertex>& GetVertices() const;

    // Get indices
    const std::vector<uint32_t>& GetIndices() const;

    // Create Vulkan buffers for this asset
    void CreateBuffers(VkDevice device, VkPhysicalDevice physicalDevice);

    // Cleanup Vulkan resources
    void Cleanup(VkDevice device);

    // Check if asset is loaded
    bool IsLoaded() const;

private:
    std::string name;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;

    bool loaded = false;
};

#endif // MERIDIAN_ASSET_H