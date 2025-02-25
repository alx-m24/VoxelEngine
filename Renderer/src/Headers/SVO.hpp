#pragma once
#ifndef SVO_H
#define SVO_H

#include "Terrain.hpp"
#include <array>
#include <glm/glm.hpp>
#include <vector>
#include <queue>

struct SVO
{
	bool isLeaf = false;
	float size = 0.0f;
	unsigned int chunkIdx = 0;
	glm::vec3 lowerCorner = glm::vec3(0.0f);
	std::vector<SVO> children;
};

class SVOSystem
{
private:
	unsigned int svoSSBO;
	unsigned int voxelsSSBO;

public:
	// Along one axis (TODO: experiment with this value)
	const int voxelCountLimit = 8;
	unsigned int maxDepth;

private:
	std::vector<SVO> SVOs;
	std::vector<glm::vec4> linearizedSVOs;
	glm::vec4* voxelPtr = nullptr;

public:
	SVOSystem(unsigned int svoSSBO, unsigned int voxelsSSBO);

public:
	void updateSVOs(std::array<glm::vec4, chunkNum>* chunks);

private:
	void updateSVO(std::array<glm::vec4, chunkNum>* chunks, unsigned int chunkIdx);
	void subdivideSVOnode(SVO& node, const glm::vec3 minIdx, const glm::vec3 maxIdx, const unsigned int chunkIdx);
	void linearizeSVOs(std::array<glm::vec4, chunkNum>* chunks);
	void linearizeSVO(SVO& node);
	bool isLeaf(const glm::vec3 minIdx, const glm::vec3 maxIdx, const unsigned int chunkIdx);

private:
	glm::vec4 toVec4(SVO& svo);
};

#endif