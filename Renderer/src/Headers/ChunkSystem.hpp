#pragma once
#ifndef CHUNKSYSTEM_H
#define CHUNKSYSTEM_H

#include "Camera.hpp"
#include "Terrain.hpp"

class ChunkSystem
{
public:
	ChunkSystem(Camera& player, std::array<glm::vec4, chunkNum>* chunks, unsigned int voxelSSBO, unsigned int chunkSSBO, unsigned int seed);

private:
	Camera& player;
	Terrain terrain{0, 0};
	std::array<glm::vec4, chunkNum>* chunks;
	glm::vec4 chunkCenter;
	unsigned int chunkSSBO;

private:
	void reloadChunks();

public:
	void update();
};

#endif // !CHUNKSYSTEM_H
