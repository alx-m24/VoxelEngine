#include "ChunkSystem.hpp"

ChunkSystem::ChunkSystem(Camera& player, std::array<glm::vec4, chunkNum>* chunks, unsigned int voxelSSBO, unsigned int svoSSBO, unsigned int chunkSSBO, unsigned int seed) : player(player), chunks(chunks), chunkSSBO(chunkSSBO) {
	terrain = Terrain(seed, voxelSSBO);

	chunks->fill(glm::vec4(0.0f));

	svoSystem = new SVOSystem(svoSSBO, voxelSSBO);

	float chunkMaxPos = ChunkSize * VoxelSize;
	chunkCenter = glm::vec4(glm::round(player.Position / chunkMaxPos) * chunkMaxPos, 1.0f);


	reloadChunks();
}

void ChunkSystem::reloadChunks()
{
	float chunkMaxPos = ChunkSize * VoxelSize;

	if (chunkNum == 1) {
		(*chunks)[0] = (chunkCenter - chunkMaxPos / 2.0f) + (chunkMaxPos * glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
		(*chunks)[0].y = 0.0f;
	}
	else {
		int chunkIdx = 0;
		for (int i = -renderRadius; i < renderRadius; ++i) {
			for (int j = -renderRadius; j < renderRadius; ++j) {
				(*chunks)[chunkIdx] = (chunkCenter - chunkMaxPos / 2.0f) + (chunkMaxPos * glm::vec4(i, 0.0f, j, 0.0f));
				(*chunks)[chunkIdx].y = 0.0f;
				++chunkIdx;
			}
		}
	}

	terrain.generate(chunks, player.Position);
	svoSystem->updateSVOs(chunks);
	sendChunks();
}

void ChunkSystem::sendChunks()
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, chunkSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, chunks->size() * 16, chunks->data(), GL_DYNAMIC_DRAW);
}

void ChunkSystem::update()
{
	glm::vec3 chunkCenter_2d = chunkCenter;
	chunkCenter_2d.y = 0.0f;

	glm::vec3 playerPos_2d = player.Position;
	playerPos_2d.y = 0.0f;


	if (glm::distance(chunkCenter_2d, playerPos_2d) > ((ChunkSize / 2.0f) * VoxelSize) * 1.0f) {
		float chunkMaxPos = ChunkSize * VoxelSize;
		//chunkCenter = glm::vec4(glm::round(player.Position / chunkMaxPos) * chunkMaxPos, 1.0f);

		//reloadChunks();
	}
}
