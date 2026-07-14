#pragma once
#include "VoxelEngine/Bindable/Bindable.hpp"
#include "VoxelEngine/utils/types.hpp"
#include <cstddef>
#include <cstdint>

namespace VoxelEngine {
    class GLBuffer : public Bindable {
        public:
            explicit GLBuffer(BufferTarget target);
            ~GLBuffer() override;

            GLBuffer(const GLBuffer&) = delete;
            GLBuffer& operator=(const GLBuffer&) = delete;
            GLBuffer(GLBuffer&&) = delete;
            GLBuffer& operator=(GLBuffer&&) = delete;

            void Bind() const override;
            void Unbind() const override;
            void SetData(const void* data, size_t size, BufferUsage usage = BufferUsage::STATIC);

            uint32_t GetID() const { return m_id; }

        private:
            mutable uint32_t m_id = 0;
            BufferTarget m_target;
    };
}
