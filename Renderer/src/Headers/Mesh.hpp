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
	unsigned int VAO;

public:
	Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures);

public:
	// Don't forget to activate/use shader first
	void draw(Shader& shader);
		
private:
	unsigned int VBO, EBO;

	void setupMesh();
};

#endif // !MESH_H
