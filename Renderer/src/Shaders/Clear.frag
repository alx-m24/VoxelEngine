#version 430 core

const int ChunkSize = 16 * 4; // Number of voxels per chunk
const float VoxelSize = 1.0 / 4.0;
const int VoxelNum = ChunkSize * ChunkSize * ChunkSize;

const int renderRadius = 2;
const int numberOfChunksInAStraightLine = (2 * renderRadius + 1);
//const int chunkNum = numberOfChunksInAStraightLine * numberOfChunksInAStraightLine;
const int chunkNum = 1;

layout(std140, binding = 0) buffer Chunks {
    vec4 chunks[chunkNum]; // vec4(posX, posY, posZ, first svo index);
};

layout(std140, binding = 1) buffer Voxels {
    vec4 voxels[VoxelNum * chunkNum]; // vec4(r, g, b, alpha + reflection) 
};

in vec2 TexCoords;
in vec3 worldPos;

uniform sampler2D texture_diffuse1;

uint toIdx(vec3 position) {
	return int(position.x) + (int(position.y) * ChunkSize) + (int(position.z) * ChunkSize * ChunkSize);
}
vec3 toGridPos(vec3 position) {
	return position / VoxelSize;
}
bool isValid(vec3 pos) {
	return (pos.x >= 0.0 && pos.y >= 0.0 && pos.z >= 0.0) && (pos.x < ChunkSize && pos.y < ChunkSize && pos.z < ChunkSize);
}

void main() {
    vec3 voxelIndex = toGridPos(worldPos - chunks[0].xyz);

    uint idx = toIdx(voxelIndex);

    if (isValid(voxelIndex)) {
        voxels[idx] = vec4(0.0);
    }

    discard;
}