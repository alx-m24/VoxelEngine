#pragma once

namespace VoxelEngine {
    enum class Type {
        FLOAT,
        INT,
        UNSIGNED_INT,
        BYTE,
        UNSIGNED_BYTE,
        SHORT,
        UNSIGNED_SHORT
    };

    uint32_t toGLType(Type type);

    enum class BufferTarget {
        VERTEX, ELEMENT, SHADER_STORAGE, UNIFORM
    };
    uint32_t toGLTarget(BufferTarget target);

    enum class BufferUsage {
        STATIC, DYNAMIC, STREAM
    };
    uint32_t toGLUsage(BufferUsage usage);
}
