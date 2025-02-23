#include "SVO.hpp"

SVOSystem::SVOSystem(unsigned int svoSSBO, unsigned int voxelsSSBO) : svoSSBO(svoSSBO), voxelsSSBO(voxelsSSBO)
{
	// Count the amount of child until the size reaches the voxelCountLimit
	int depth = 0;
	float size = ChunkSize * VoxelSize;
	int voxelNum = size / VoxelSize;
	while (voxelNum > voxelCountLimit) {
		++depth;

		size /= 2.0f;
		voxelNum = size / VoxelSize;
	}

	maxDepth = depth;

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, svoSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, maxDepth * 8 * 16, nullptr, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, svoSSBO);
}

void SVOSystem::updateSVOs(std::array<glm::vec4, chunkNum>* chunks)
{
	SVOs.clear();
	linearizedSVOs.clear();

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, voxelsSSBO);
	voxelPtr = static_cast<glm::vec4*>(glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY));

	//std::vector<std::thread> threads;

	for (int i = 0; i < chunks->size(); ++i) {
		updateSVO(chunks, i); // single threaded
		//threads.emplace_back(&SVOSystem::updateSVO, &(*this), chunks, i); // threaded
	}

	/*
	for (std::thread& thread : threads) {
		if (thread.joinable()) {
			thread.join();
		}
	}*/

	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

	linearizeSVOs(chunks);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, svoSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, linearizedSVOs.size() * 16, linearizedSVOs.data(), GL_DYNAMIC_DRAW);
}

void SVOSystem::updateSVO(std::array<glm::vec4, chunkNum>* chunks, unsigned int chunkIdx)
{
	SVO& head = SVOs.emplace_back(SVO());

	head.chunkIdx = chunkIdx;

	head.isLeaf = true;
	head.lowerCorner = glm::vec3((*chunks)[chunkIdx]);
	head.size = ChunkSize * VoxelSize;

	// Try threading here maybe
	subdivideSVOnode(head, glm::vec3(0.0f), glm::vec3(16.0f), chunkIdx);
}

void SVOSystem::subdivideSVOnode(SVO& node, const glm::vec3 minIdx, const glm::vec3 maxIdx, const unsigned int chunkIdx)
{
	node.children.resize(8);

	const float voxelNum = node.size / VoxelSize; // Along one axis
	
	node.isLeaf = isLeaf(minIdx, maxIdx, chunkIdx) || voxelNum <= voxelCountLimit;

	if (node.isLeaf) {
		//std::cout << "leaf: " << node.size << std::endl;
		return;
	}

	int index = 0;
	float newSize = node.size / 2.0f;
	float newVoxelNum = newSize / VoxelSize; // Along one axis
	glm::vec3 axes = glm::vec3(0.0f);

	for (axes.x = 0; axes.x <= 1; ++axes.x) {
		for (axes.y = 0; axes.y <= 1; ++axes.y) {
			for (axes.z = 0; axes.z <= 1; ++axes.z) {
				node.children[index].isLeaf = false;
				node.children[index].lowerCorner = node.lowerCorner + newSize * axes;
				node.children[index].size = newSize;

				glm::vec3 newMinIdx = minIdx + newVoxelNum * axes;
				glm::vec3 newMaxIdx = newMinIdx + glm::vec3(newVoxelNum);

				// Try threading here maybe
				subdivideSVOnode(node.children[index], newMinIdx, newMaxIdx, chunkIdx);

				++index;
			}
		}
	}
}

void SVOSystem::linearizeSVOs(std::array<glm::vec4, chunkNum>* chunks)
{
	for (SVO& root : SVOs) {
		(*chunks)[root.chunkIdx].a = linearizedSVOs.size();

		linearizeSVO(root);
	}
}

void SVOSystem::linearizeSVO(SVO& node)
{
	std::queue<SVO*> q;
	q.push(&node);

	while (!q.empty()) {
		SVO* current = q.front();
		q.pop();

		// Pushing to 'result' vector
		linearizedSVOs.emplace_back(toVec4(*current));

		//std::cout << linearizedSVOs.back().a << ", ";

		// Enqueue children if it's not a leaf
		if (!current->isLeaf) {
			for (SVO& child : current->children) {
				q.push(&child);
			}
		}
	}
}

bool SVOSystem::isLeaf(const glm::vec3 minIdx, const glm::vec3 maxIdx, const unsigned int chunkIdx)
{
	for (int x = minIdx.x; x < maxIdx.x; ++x) {
		for (int y = minIdx.y; y < maxIdx.y; ++y) {
			for (int z = minIdx.z; z < maxIdx.z; ++z) {
				int idx = toIdx(glm::vec3(x, y, z)) + chunkIdx * VoxelNum;

				if (voxelPtr[idx].a != 0.0f) {
					return false;
				}
			}
		}
	}

	return true;
}

glm::vec4 SVOSystem::toVec4(SVO& svo)
{
	glm::vec4 linearSVO = glm::vec4(svo.lowerCorner, svo.size);

	if (svo.isLeaf) linearSVO.a = -linearSVO.a;

	return linearSVO;
}