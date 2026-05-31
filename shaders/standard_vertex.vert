#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

layout(location = 0) out vec3 fragp;
layout(location = 1) out vec3 normp;
layout(location = 2) out vec2 uvp;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(push_constant) uniform PushConstants {
    mat4 model;
} pc;

void main() {

    fragp = vec3(pc.model * vec4(position, 1.0));
    normp = mat3(pc.model) * normal;
    uvp = uv;

    gl_Position = ubo.proj * ubo.view * pc.model * vec4(position, 1.0);
}