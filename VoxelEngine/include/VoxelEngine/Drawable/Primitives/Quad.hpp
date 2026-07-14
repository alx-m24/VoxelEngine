#pragma once
#include "VoxelEngine/Bindable/Bindable.hpp"
#include "VoxelEngine/Drawable/Drawable.hpp"
#include "VoxelEngine/Buffer/VAO.hpp"

namespace VoxelEngine {
    class Quad : public Bindable, public Drawable {
        public:
            Quad();
            ~Quad() override = default;

            Quad(const Quad&) = delete;
            Quad& operator=(const Quad&) = delete;

            void Bind() const override;
            void Unbind() const override;
            void Draw() const override;

        private:
            VertexArray   m_vao;
            VertexBuffer  m_vbo;
            ElementBuffer m_ebo;
    };
}
