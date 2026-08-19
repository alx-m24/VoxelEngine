#version 460 core

in vec2 vUV;
out vec4 FragColor;

uniform sampler2D u_voxelColor;
uniform sampler2D u_voxelDepth;
uniform sampler2D u_rasterColor;
uniform sampler2D u_rasterDepth;

void main() {
    vec3  voxelCol   = texture(u_voxelColor, vUV).rgb;
    float voxelDepth = texture(u_voxelDepth, vUV).r;

    vec3  rasterCol   = texture(u_rasterColor, vUV).rgb;
    float rasterDepth = texture(u_rasterDepth, vUV).r;

    // nearer depth wins
    FragColor = (voxelDepth < rasterDepth)
        ? vec4(voxelCol, 1.0)
        : vec4(rasterCol, 1.0);
}
