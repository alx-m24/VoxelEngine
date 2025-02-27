#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

uniform mat4 model;

out vec2 TexCoords;
out vec3 worldPos;

void main() {
	TexCoords = aTexCoords;

	worldPos = vec3(model * vec4(aPos, 1.0));

	gl_Position = vec4(aPos, 1.0);
}