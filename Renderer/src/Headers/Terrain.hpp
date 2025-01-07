#pragma once
#ifndef TERRAIN_H
#define TERRAIN_H

#include "PerlinNoise/PerlinNoise.hpp"
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <thread>
#include <iostream>
#include "MyFuncs/myFuncs.hpp"

constexpr unsigned int ChunkSize = 16 * 4; // Number of voxels per chunk
constexpr float VoxelSize = 1.0f / 4.0f;
constexpr int VoxelNum = ChunkSize * ChunkSize * ChunkSize;

constexpr int renderRadius = 8;
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

	void generate(std::array<glm::vec4, chunkNum>* chunks, glm::vec3 cameraPos);
	void generateChunk(glm::vec3 cameraPos, std::array<glm::vec4, chunkNum>* chunks, int chunkIdx);

public:
	glm::vec4 getColor(glm::vec3 idx, float ChunkSize);

private:
	glm::vec4* voxelsPtr = nullptr;
	glm::vec4* chunkPtr = nullptr;
	siv::PerlinNoise::seed_type Seed;
	siv::PerlinNoise perlin;
	unsigned int voxelSSBO;
};

#endif // !TERRAIN_H