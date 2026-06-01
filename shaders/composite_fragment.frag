#version 450

layout(set = 0, binding = 0) uniform sampler2D hdrScene;
layout(set = 0, binding = 1) uniform sampler2D bloomScene;

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

vec3 tonemap(vec3 x) {
    return x / (x + vec3(1.0));
}

void main() {
    vec3 hdrColor = texture(hdrScene, inUV).rgb;
    vec3 bloomColor = texture(bloomScene, inUV).rgb;

    vec3 color = hdrColor + bloomColor;
    color = tonemap(color);

    outColor = vec4(color, 1.0);
}