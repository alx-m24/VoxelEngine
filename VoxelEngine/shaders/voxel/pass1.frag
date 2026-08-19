#version 330 core

in vec2 vUV;

out vec4 FragColor;

uniform mat4 inv_CameraMatrix;

void main() { 
    FragColor = vec4(vUV, 0.0, 1.0);
}
