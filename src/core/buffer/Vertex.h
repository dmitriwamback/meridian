//
// Created by Dmitri Wamback on 2026-05-30.
//

#ifndef MERIDIAN_VERTEX_H
#define MERIDIAN_VERTEX_H

#include <glm/glm/glm.hpp>
#include <glm/glm/vec2.hpp>
#include <glm/glm/vec3.hpp>
#include <glm/glm/vec4.hpp>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;

    static VkVertexInputBindingDescription GetBindingDescription();
    static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions();
};

inline VkVertexInputBindingDescription Vertex::GetBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescription;
}

inline std::vector<VkVertexInputAttributeDescription> Vertex::GetAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions(3);

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, position);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, normal);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex, uv);

    return attributeDescriptions;
}

#endif //MERIDIAN_VERTEX_H