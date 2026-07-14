#pragma once

#include "Shader.hpp"

namespace VoxelEngine {
    uint64_t GetMaxThreadsPerDispatch(int localSizeX, int localSizeY, int localSizeZ);

    class ComputeShader : public Shader {
        public:
            ComputeShader(const std::string& path, bool isFromFile);
    };
}
