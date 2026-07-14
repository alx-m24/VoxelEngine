#version 330 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;

out vec3 vColor;

void main() {
    vec3 colors[4] = vec3[](
        vec3(1.0, 0.0, 0.0), // vertex 0: red
        vec3(0.0, 1.0, 0.0), // vertex 1: green
        vec3(0.0, 0.0, 1.0), // vertex 2: blue
        vec3(1.0, 1.0, 0.0)  // vertex 3: yellow
    );
    vColor = colors[gl_VertexID];
    gl_Position = vec4(aPos, 0.0, 1.0);
}
