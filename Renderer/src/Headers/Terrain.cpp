#include "Terrain.hpp"

void Terrain::generate(std::array<glm::vec4, VoxelNum* chunkNum>* voxels, std::array<glm::vec4, chunkNum>* chunks, unsigned int Seed)
{
	chunks->fill(glm::vec4(0.0f));
	voxels->fill(glm::vec4(0.0f));

	float chunkMaxPos = ChunkSize * VoxelSize;

	(*chunks)[0] = glm::vec4(0.0f);
	(*chunks)[1] = glm::vec4(chunkMaxPos, 0.0f, 0.0f, 0.0f);
	(*chunks)[2] = glm::vec4(-chunkMaxPos, 0.0f, 0.0f, 0.0f);
	(*chunks)[3] = glm::vec4(0.0f, 0.0f, chunkMaxPos, 0.0f);
	(*chunks)[4] = glm::vec4(0.0f, 0.0f, -chunkMaxPos, 0.0f);
	(*chunks)[5] = glm::vec4(chunkMaxPos, 0.0f, -chunkMaxPos, 0.0f);
	(*chunks)[6] = glm::vec4(chunkMaxPos, 0.0f, chunkMaxPos, 0.0f);
	(*chunks)[7] = glm::vec4(-chunkMaxPos, 0.0f, chunkMaxPos, 0.0f);
	(*chunks)[8] = glm::vec4(-chunkMaxPos, 0.0f, -chunkMaxPos, 0.0f);

	const siv::PerlinNoise::seed_type seed = Seed;
	const siv::PerlinNoise perlin{ seed };
	const float denominator = 0.5f * (ChunkSize * VoxelSize);

	srand(time(0));
	for (int chunkIdx = 0; chunkIdx < chunkNum; ++chunkIdx) {
		glm::vec3 chunkPos = glm::vec3((*chunks)[chunkIdx]);

		for (int i = 0; i < ChunkSize; ++i) {
			for (int j = 0; j < ChunkSize; ++j) {
				for (int k = 0; k < ChunkSize; ++k) {
					glm::vec3 position = (glm::vec3(i, j, k) * VoxelSize) + (VoxelSize / 2.0f) + chunkPos;

					const double noise = perlin.noise3D_01(position.x / denominator, position.y / denominator, position.z / denominator);

					glm::vec3 index = toGridPos(position - chunkPos);

					glm::vec4 color = noise > 0.5f ?
						getColor(glm::vec3(i, j, k))
						: glm::vec4(0.0f);

					(*voxels)[toIdx(index) + chunkIdx * VoxelNum] = color;
				}
			}
		}
	}
}

glm::vec4 Terrain::getColor(glm::vec3 idx) {
	float random = (float)(rand()) / RAND_MAX;
	random /= 10.0f;

	if (idx.y >= ChunkSize - 1) return glm::vec4(0.313f + random, 0.901f + random, 0.262f + random, 1.0f);

	if (random * 10.0f < 0.01f) return glm::vec4(glm::vec3(0.701f), 1.0f);

	if (idx.y >= ChunkSize - 8) return glm::vec4(0.545f + random, 0.270f + random, 0.0745f + random, 1.0f);

	return glm::vec4(glm::vec3(0.450f + random), 1.0f);
}
