#pragma once

#include "VoxelEngine/mesh/mesh.hpp"

namespace VoxelEngine::Primitives {
    class Quad : public Mesh<uint16_t> {
        public:
            Quad() {
                vertices = {
                    {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
                    {{ 1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
                    {{ 1.0f,  1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
                    {{-1.0f,  1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}}
                };

                indices = {
                    0, 1, 2,
                    0, 2, 3
                };
            }
    };
}
