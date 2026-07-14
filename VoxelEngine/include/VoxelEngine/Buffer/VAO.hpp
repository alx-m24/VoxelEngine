#pragma once

#include "VoxelEngine/Bindable/Bindable.hpp"

#include "glBuffer.hpp"
#include "Buffers.hpp"
#include "VoxelEngine/utils/types.hpp"

namespace VoxelEngine {
    struct VertexAttribute {
        unsigned int index;
        int          count;      // e.g. 3 for vec3
        Type         type;       // GL_FLOAT, etc.
        bool         normalized;
        int          stride;
        size_t       offset;
    };

    class VertexArray : public Bindable {
        public:
            VertexArray();
            ~VertexArray() override;

            VertexArray(const VertexArray&) = delete;
            VertexArray& operator=(const VertexArray&) = delete;

            void Bind() const override;
            void Unbind() const override;

            void AddVertexBuffer(const VertexBuffer& vbo, const VertexAttribute& attr);

            void SetElementBuffer(const ElementBuffer& ebo);

        private:
            unsigned int m_id = 0;
    };
}
