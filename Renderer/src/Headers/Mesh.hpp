#pragma once
#ifndef MESH_H
#define MESH_H

#include <glad/glad.h>
#include <vector>
#include <string>
#include "Shaders/Shader.hpp"
#include "Textures/Textures.hpp"
#include "Vertex/Vertex.hpp"

class Mesh {
public:
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	std::vector<Texture> textures;

public:
	Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures);

public:
	void draw(Shader& shader);

private:
	unsigned int VAO, VBO, EBO;

	void setupMesh();
};

#endif // !MESH_H
