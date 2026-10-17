#version 450

layout(location = 0) in vec3 vNormal;
layout(location = 1) in vec3 vColor;

layout(location = 0) out vec4 outColor;

void main() {
    const vec3 lightDir = normalize(vec3(0.4, 0.8, 0.6));   // direction *towards* the light
    float diffuse = max(dot(normalize(vNormal), lightDir), 0.0);
    outColor = vec4(vColor * (0.25 + 0.75 * diffuse), 1.0);
}
