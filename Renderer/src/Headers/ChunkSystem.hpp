#pragma once
#ifndef CHUNKSYSTEM_H
#define CHUNKSYSTEM_H

#include "Camera.hpp"
#include "Terrain.hpp"
#include <thread>

class ChunkSystem
{
public:
	ChunkSystem(Camera& player, std::array<glm::vec4, chunkNum>* chunks, unsigned int voxelSSBO, unsigned int chunkSSBO, unsigned int seed);

private:
	unsigned int chunkSSBO;
	Camera& player;
	Terrain terrain{0, 0};
	std::array<glm::vec4, chunkNum>* chunks;

private:
	void reloadChunks();

public:
	void update();
};

#endif // !CHUNKSYSTEM_H
