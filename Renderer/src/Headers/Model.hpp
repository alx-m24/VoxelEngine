#pragma once
#ifndef MODEL_H
#define MODEL_H

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <string>
#include <vector>
#include "Mesh.hpp"
#include "Shaders/Shader.hpp"

class Model {
public:
	Model(std::string path);

public:
	void draw(Shader& shader);

private:
	std::vector<Texture> textures_loaded;
	std::vector<Mesh> meshes;
	std::string directory;

private:
	void loadModel(std::string path);
	void processNode(aiNode* node, const aiScene* scene);

	Mesh processMesh(aiMesh* mesh, const aiScene* scene);
	std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);
};

#endif // !MODEL_H
