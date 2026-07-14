#include "pch.h"
#include "VoxelEngine/Buffer/Buffers.hpp"
#include <glad/glad.h>

namespace VoxelEngine {
    void ShaderStorageBuffer::BindBase(unsigned int bindingPoint) const {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, GetID());
    }
    void UniformBuffer::BindBase(unsigned int bindingPoint) const {
        glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, GetID());
    }
}
