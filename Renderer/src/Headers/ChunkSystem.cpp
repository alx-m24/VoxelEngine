#include "ChunkSystem.hpp"

ChunkSystem::ChunkSystem(Camera& player, std::array<glm::vec4, chunkNum>* chunks, unsigned int voxelSSBO, unsigned int chunkSSBO, unsigned int seed) : player(player), chunks(chunks), chunkSSBO(chunkSSBO) {
	terrain = Terrain(seed, voxelSSBO);

	chunks->fill(glm::vec4(0.0f));

	reloadChunks();
}

void ChunkSystem::reloadChunks()
{
	float chunkMaxPos = ChunkSize * VoxelSize;

	(*chunks)[0] = glm::vec4(glm::round(player.Position / chunkMaxPos) * chunkMaxPos, 1.0f) - chunkMaxPos / 2.0f;
	(*chunks)[0].y = 0.0f;

	(*chunks)[1] = glm::vec4(chunkMaxPos, 0.0f, 0.0f, 0.0f) + (*chunks)[0];
	(*chunks)[2] = glm::vec4(-chunkMaxPos, 0.0f, 0.0f, 0.0f) + (*chunks)[0];
	(*chunks)[3] = glm::vec4(0.0f, 0.0f, chunkMaxPos, 0.0f) + (*chunks)[0];
	(*chunks)[4] = glm::vec4(0.0f, 0.0f, -chunkMaxPos, 0.0f) + (*chunks)[0];
	(*chunks)[5] = glm::vec4(chunkMaxPos, 0.0f, -chunkMaxPos, 0.0f) + (*chunks)[0];
	(*chunks)[6] = glm::vec4(chunkMaxPos, 0.0f, chunkMaxPos, 0.0f) + (*chunks)[0];
	(*chunks)[7] = glm::vec4(-chunkMaxPos, 0.0f, chunkMaxPos, 0.0f) + (*chunks)[0];
	(*chunks)[8] = glm::vec4(-chunkMaxPos, 0.0f, -chunkMaxPos, 0.0f) + (*chunks)[0];

	terrain.generate(chunks);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, chunkSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, chunks->size() * 16, chunks->data(), GL_DYNAMIC_DRAW);
}

void ChunkSystem::update()
{
	glm::vec3 chunkCenter = (*chunks)[0] + (ChunkSize / 2.0f) * VoxelSize;

	glm::vec3 chunkCenter_2d = chunkCenter;
	chunkCenter_2d.y = 0.0f;

	glm::vec3 playerPos_2d = player.Position;
	playerPos_2d.y = 0.0f;


	if (glm::distance(chunkCenter_2d, playerPos_2d) > ((ChunkSize / 2.0f) * VoxelSize) * 1.1f) {
		reloadChunks();
	}
}
