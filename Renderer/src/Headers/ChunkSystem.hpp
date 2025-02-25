#pragma once
#ifndef CHUNKSYSTEM_H
#define CHUNKSYSTEM_H

#include "Camera.hpp"
#include "Terrain.hpp"
#include "SVO.hpp"

class ChunkSystem
{
public:
	ChunkSystem(Camera& player, std::array<glm::vec4, chunkNum>* chunks, unsigned int voxelSSBO, unsigned int svoSSBO, unsigned int chunkSSBO, unsigned int seed);
	~ChunkSystem() { delete svoSystem; }

private:
	Camera& player;
	Terrain terrain{0, 0};
	std::array<glm::vec4, chunkNum>* chunks;
	glm::vec4 chunkCenter;
	SVOSystem* svoSystem;
	unsigned int chunkSSBO;

private:
	void sendChunks();

public:
	void reloadChunks();
	void update();
	int getSVOLimit() { return svoSystem->voxelCountLimit; };
};

#endif // !CHUNKSYSTEM_H
