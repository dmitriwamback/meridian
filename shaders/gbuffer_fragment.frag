#version 450

layout(location = 0) in vec3 fragp;
layout(location = 1) in vec3 normp;
layout(location = 2) in vec2 uvp;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outPosition;
layout(location = 3) out vec4 outMisc;

void main() {
    vec3 albedo = vec3(0.8, 0.8, 0.8);

    outColor = vec4(albedo, 1.0);
    outNormal = vec4(normalize(normp), 1.0);
    outPosition = vec4(fragp, 1.0);
    outMisc = vec4(0.0, 0.0, 0.0, 1.0); // placeholder
}