#include "pch.h"
#include "VoxelEngine/utils/types.hpp"
#include <glad/glad.h>

#include "VoxelEngine/utils/assert.hpp"

namespace VoxelEngine {
    uint32_t toGLType(Type type) {
        switch (type) {
            case Type::FLOAT:          return GL_FLOAT;
            case Type::INT:            return GL_INT;
            case Type::UNSIGNED_INT:   return GL_UNSIGNED_INT;
            case Type::BYTE:           return GL_BYTE;
            case Type::UNSIGNED_BYTE:  return GL_UNSIGNED_BYTE;
            case Type::SHORT:          return GL_SHORT;
            case Type::UNSIGNED_SHORT: return GL_UNSIGNED_SHORT;
        }
        V_ASSERT(false, "toGLType: unhandled Type");
        return 0;
    }

    uint32_t toGLTarget(BufferTarget target) {
        switch (target) {
            case BufferTarget::VERTEX:         return GL_ARRAY_BUFFER;
            case BufferTarget::ELEMENT:        return GL_ELEMENT_ARRAY_BUFFER;
            case BufferTarget::SHADER_STORAGE: return GL_SHADER_STORAGE_BUFFER;
            case BufferTarget::UNIFORM:        return GL_UNIFORM_BUFFER;
        }
        V_ASSERT(false, "toGLTarget: unhandled BufferTarget");
        return 0;
    }
    
    uint32_t toGLUsage(BufferUsage usage) {
        switch (usage) {
            case BufferUsage::STATIC:  return GL_STATIC_DRAW;
            case BufferUsage::DYNAMIC: return GL_DYNAMIC_DRAW;
            case BufferUsage::STREAM:  return GL_STREAM_DRAW;
        }
        V_ASSERT(false, "toGLUsage: unhandled BufferUsage");
        return 0;
    }
}
