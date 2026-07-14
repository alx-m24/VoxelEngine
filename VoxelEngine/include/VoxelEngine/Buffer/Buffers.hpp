#pragma once
#include "VoxelEngine/Buffer/glBuffer.hpp"
#include "VoxelEngine/utils/types.hpp"

namespace VoxelEngine {

    class VertexBuffer : public GLBuffer {
        public: VertexBuffer() : GLBuffer(BufferTarget::VERTEX) {}
    };

    class ElementBuffer : public GLBuffer {
        public:
            ElementBuffer() : GLBuffer(BufferTarget::ELEMENT) {}
            void SetData(const unsigned int* indices, size_t count, BufferUsage usage = BufferUsage::STATIC) {
                GLBuffer::SetData(indices, count * sizeof(unsigned int), usage);
                m_count = static_cast<unsigned int>(count);
            }
            unsigned int GetCount() const { return m_count; }
        private:
            unsigned int m_count = 0;
    };

    class ShaderStorageBuffer : public GLBuffer {
        public:
            ShaderStorageBuffer() : GLBuffer(BufferTarget::SHADER_STORAGE) {}
            void BindBase(unsigned int bindingPoint) const;
    };

    class UniformBuffer : public GLBuffer {
        public:
            UniformBuffer() : GLBuffer(BufferTarget::UNIFORM) {}
            void BindBase(unsigned int bindingPoint) const;
    };
}
