#version 450

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform Push {
    vec2 resolution;                 // framebuffer size in pixels
} pc;

void main() {
    vec2 uv = gl_FragCoord.xy / pc.resolution.xy;
    outColor = vec4(uv, 0.0, 1.0);
}
