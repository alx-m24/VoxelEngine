#pragma once
#ifndef TERRAIN_H
#define TERRAIN_H

#include "PerlinNoise/PerlinNoise.hpp"
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <iostream>

constexpr unsigned int ChunkSize = 16 * 1; // Number of voxels per chunk
constexpr float VoxelSize = 1.0f / 1.0f;
constexpr int VoxelNum = ChunkSize * ChunkSize * ChunkSize;

constexpr int renderRadius = 1;
constexpr int numberOfChunksInAStraightLine = (2 * renderRadius + 1);
constexpr int chunkNum = numberOfChunksInAStraightLine * numberOfChunksInAStraightLine;

static constexpr unsigned int toIdx(glm::vec3 position) {
	return static_cast<int>(position.x) + static_cast<int>(position.y) * ChunkSize + static_cast<int>(position.z) * ChunkSize * ChunkSize;
}
static constexpr glm::vec3 toGridPos(glm::vec3 position) {
	return position / VoxelSize;
}
static bool isValid(glm::vec3 pos) {
	return (pos.x >= 0 && pos.y >= 0 && pos.z >= 0) && (pos.x < ChunkSize && pos.y < ChunkSize && pos.z < ChunkSize);
}

class Terrain
{
public:
	Terrain(unsigned int seed, unsigned int voxelSSBO);

	void generate(std::array<glm::vec4, chunkNum>* chunks);
	glm::vec4 getColor(glm::vec3 idx);

private:
	siv::PerlinNoise::seed_type Seed;
	siv::PerlinNoise perlin;
	unsigned int voxelSSBO;
};

#endif // !TERRAIN_H