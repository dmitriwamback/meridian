#version 450

layout(location = 0) out vec4 fragc;

layout(location = 0) in vec3 fragp;
layout(location = 1) in vec3 normp;
layout(location = 2) in vec2 uvp;

vec3 lightDirection = normalize(vec3(1.0, -1.0, -0.32));
vec3 lightColor = vec3(1.0, 1.0, 1.0);
float lightIntensity = 1.0;

vec3 ambientColor = vec3(0.2, 0.2, 0.2);
float ambientIntensity = 0.3;

vec3 materialColor = vec3(0.7, 0.8, 0.9);

void main() {
    vec3 normal = normalize(normp);

    float diff = max(dot(normal, -lightDirection), 0.0);
    vec3 diffuse = diff * lightColor * lightIntensity;

    vec3 lighting = ambientColor + diffuse * ambientIntensity;

    vec3 finalColor = materialColor * lighting;

    fragc = vec4(finalColor, 1.0);
}