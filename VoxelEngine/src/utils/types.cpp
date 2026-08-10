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

    GLFormatInfo toGLFormat(TextureFormat format) {
        switch (format) {
            case TextureFormat::R8:               return { GL_R8,               GL_RED,             GL_UNSIGNED_BYTE };
            case TextureFormat::R16F:              return { GL_R16F,             GL_RED,             GL_FLOAT };
            case TextureFormat::R32F:              return { GL_R32F,             GL_RED,             GL_FLOAT };
            case TextureFormat::R32UI:             return { GL_R32UI,            GL_RED_INTEGER,     GL_UNSIGNED_INT };
            case TextureFormat::R32I:              return { GL_R32I,             GL_RED_INTEGER,     GL_INT };
            case TextureFormat::RGB8:              return { GL_RGB8,             GL_RGB,             GL_UNSIGNED_BYTE };
            case TextureFormat::RGBA8:             return { GL_RGBA8,            GL_RGBA,            GL_UNSIGNED_BYTE };
            case TextureFormat::RGB16F:             return { GL_RGB16F,           GL_RGB,             GL_FLOAT };
            case TextureFormat::RGBA16F:            return { GL_RGBA16F,          GL_RGBA,            GL_FLOAT };
            case TextureFormat::RGBA32F:            return { GL_RGBA32F,          GL_RGBA,            GL_FLOAT };
            case TextureFormat::DEPTH24:            return { GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_FLOAT };
            case TextureFormat::DEPTH32F:           return { GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT };
            case TextureFormat::DEPTH24_STENCIL8:   return { GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL,   GL_UNSIGNED_INT_24_8 };
        }
        V_ASSERT(false, "toGLFormat: unhandled TextureFormat");
        return {};
    }

    uint32_t toGLFilter(TextureFilter filter) {
        switch (filter) {
            case TextureFilter::NEAREST: return GL_NEAREST;
            case TextureFilter::LINEAR:  return GL_LINEAR;
        }
        V_ASSERT(false, "toGLFilter: unhandled TextureFilter");
        return 0;
    }

    uint32_t toGLWrap(TextureWrap wrap) {
        switch (wrap) {
            case TextureWrap::REPEAT:          return GL_REPEAT;
            case TextureWrap::MIRRORED_REPEAT: return GL_MIRRORED_REPEAT;
            case TextureWrap::CLAMP_TO_EDGE:   return GL_CLAMP_TO_EDGE;
            case TextureWrap::CLAMP_TO_BORDER: return GL_CLAMP_TO_BORDER;
        }
        V_ASSERT(false, "toGLWrap: unhandled TextureWrap");
        return 0;
    }
}
