#include "pch.h"
#include "VoxelEngine/Drawable/Primitives/Quad.hpp"
#include "VoxelEngine/Bindable/SmartBinder.hpp"

#include <glad/glad.h>

namespace VoxelEngine {

    namespace {
        // NDC quad, position (vec2) + uv (vec2) interleaved
        constexpr float k_vertices[] = {
            // pos          // uv
            -1.0f, -1.0f,   0.0f, 0.0f,
             1.0f, -1.0f,   1.0f, 0.0f,
             1.0f,  1.0f,   1.0f, 1.0f,
            -1.0f,  1.0f,   0.0f, 1.0f,
        };
        constexpr unsigned int k_indices[] = {
            0, 1, 2,
            2, 3, 0,
        };
    }

    Quad::Quad() {
        m_vbo.SetData(k_vertices, sizeof(k_vertices));
        m_ebo.SetData(k_indices, 6);

        m_vao.AddVertexBuffer(m_vbo, VertexAttribute{
            .index = 0, .count = 2, .type = VoxelEngine::Type::FLOAT,
            .normalized = false, .stride = 4 * sizeof(float), .offset = 0
        });
        m_vao.AddVertexBuffer(m_vbo, VertexAttribute{
            .index = 1, .count = 2, .type = Type::FLOAT,
            .normalized = false, .stride = 4 * sizeof(float), .offset = 2 * sizeof(float)
        });
        m_vao.SetElementBuffer(m_ebo);
    }

    void Quad::Bind() const   { m_vao.Bind(); }
    void Quad::Unbind() const { m_vao.Unbind(); }

    void Quad::Draw() const {
        SmartBinder bind(*this);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_ebo.GetCount()), GL_UNSIGNED_INT, nullptr);
    }
}
