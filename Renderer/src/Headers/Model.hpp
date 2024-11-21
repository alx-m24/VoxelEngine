#pragma once
#ifndef MODEL_H
#define MODEL_H

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <vector>
#include <unordered_map>
#include "Mesh.hpp"
#include "Shaders/Shader.hpp"

struct Transformations {
	glm::vec3 position = { 0.0f, 0.0f, 0.0f };
	glm::vec3 rotation = { 0.0f, 0.0f, 0.0f };
	glm::vec3 scale = { 1.0f, 1.0f, 1.0f };
};

glm::mat4 getModelMatrix(Transformations& transformation);

class Model {
public:
	Model(std::string path);

public:
	std::vector<Mesh> meshes;

private:
	std::vector<Texture> textures_loaded;
	std::string directory;

private:
	void loadModel(std::string path);
	void processNode(aiNode* node, const aiScene* scene);
	
	Mesh processMesh(aiMesh* mesh, const aiScene* scene);
	std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);
};

#endif // !MODEL_H
