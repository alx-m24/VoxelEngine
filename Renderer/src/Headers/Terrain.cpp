#include "Terrain.hpp"

Terrain::Terrain(unsigned int seed, unsigned int voxelSSBO) : voxelSSBO(voxelSSBO)
{
	Seed = siv::PerlinNoise::seed_type(seed);
	perlin = siv::PerlinNoise(Seed);

	srand(seed);
}

void Terrain::generate(std::array<glm::vec4, chunkNum>* chunks, glm::vec3 cameraPos)
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, voxelSSBO);
	voxelsPtr = static_cast<glm::vec4*>(glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE));

	std::vector<std::thread> threads;

	for (int chunkIdx = 0; chunkIdx < chunkNum; ++chunkIdx) {
		threads.emplace_back(std::thread(&Terrain::generateChunk, &(*this), cameraPos, chunks, chunkIdx));
		//generateChunk(cameraPos, chunks, chunkIdx);
	}

	for (std::thread& thread : threads) {
		if (thread.joinable()) {
			thread.join();
		}
	}

	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
}

void Terrain::generateChunk(glm::vec3 cameraPos, std::array<glm::vec4, chunkNum>* chunks, int chunkIdx)
{
	glm::vec3 chunkPos = glm::vec3((*chunks)[chunkIdx]);

	const float denominator = 0.5f * (ChunkSize * VoxelSize);

	for (int i = 0; i < ChunkSize; ++i) {
		for (int j = 0; j < ChunkSize; ++j) {
			for (int k = 0; k < ChunkSize; ++k) {
				glm::vec3 position = (glm::vec3(i, j, k) * VoxelSize) + (VoxelSize / 2.0f) + chunkPos;

				const double noise = perlin.noise3D_01(position.x / denominator, position.y / denominator, position.z / denominator);

				glm::vec3 index = toGridPos(position - chunkPos);
				unsigned int idx = toIdx(index) + chunkIdx * VoxelNum;

				if (voxelsPtr[idx] != glm::vec4(0.0f)) continue;

				glm::vec4 color = (noise > 0.5f) ? getColor(glm::vec3(i, j, k), ChunkSize) : glm::vec4(0.0f);

				voxelsPtr[idx] = color;

				if (idx == 0 && color.a == 0.0f) voxelsPtr[0].a = -0.01f;
			}
		}
	}
}

glm::vec4 Terrain::getColor(glm::vec3 idx, float ChunkSize) {
	float random = (float)(rand()) / RAND_MAX;
	random /= 10.0f;

	if (idx.y >= ChunkSize - 1) return glm::vec4(0.313f + random, 0.901f + random, 0.262f + random, 1.0f);

	if (random * 10.0f < 0.01f) return glm::vec4(glm::vec3(0.701f), 1.0f);

	if (idx.y >= ChunkSize - 8) return glm::vec4(0.545f + random, 0.270f + random, 0.0745f + random, 1.0f);

	return glm::vec4(glm::vec3(0.450f + random), 1.0f);
}
