#pragma once

#include <array>

#include <glm/glm.hpp>

#include "VoxelEngine/mesh/mesh.hpp"

namespace VoxelEngine::Primitives {
    struct Face { glm::vec3 n, u, v, color; };        // u x v == n, so (u right, v up) seen from outside

    namespace Test {
    inline constexpr std::array<Face, 6u> faces = {
            Face{ { 1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {0.90f, 0.30f, 0.30f} },
            Face{ {-1, 0, 0}, {0, 0, 1}, {0, 1, 0}, {0.30f, 0.80f, 0.80f} },
            Face{ { 0, 1, 0}, {0, 0, 1}, {1, 0, 0}, {0.40f, 0.85f, 0.40f} },
            Face{ { 0,-1, 0}, {1, 0, 0}, {0, 0, 1}, {0.85f, 0.40f, 0.85f} },
            Face{ { 0, 0, 1}, {1, 0, 0}, {0, 1, 0}, {0.35f, 0.45f, 0.95f} },
            Face{ { 0, 0,-1}, {0, 1, 0}, {1, 0, 0}, {0.95f, 0.85f, 0.35f} } };
    }

    inline constexpr glm::vec2 corners[] = { {-1, -1}, {1, -1}, {1, 1}, {-1, 1} };   // counter-clockwise

    class Cube : public VoxelEngine::Mesh<uint16_t> {
        public:
            Cube(const std::array<Face, 6u>& faces = Test::faces) {
                for (const Face& f : faces) {
                    const auto base = static_cast<uint16_t>(vertices.size());
                    for (const glm::vec2& c : corners)
                        vertices.push_back({ 0.5f * (f.n + c.x * f.u + c.y * f.v), f.n, f.color });
                    for (uint16_t i : { 0, 1, 2, 0, 2, 3 }) indices.push_back(static_cast<uint16_t>(base + i));
                }
            }
    };
}
