#pragma once

#include <cstdint>

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

    enum class TextureFormat {
        R8, R16F, R32F, R32UI, R32I,
        RGB8, RGBA8,
        RGB16F, RGBA16F, RGBA32F,
        DEPTH24, DEPTH32F, DEPTH24_STENCIL8
    };

    struct GLFormatInfo {
        uint32_t internalFormat;
        uint32_t format;
        uint32_t type;
    };
    GLFormatInfo toGLFormat(TextureFormat format);

    enum class TextureFilter { NEAREST, LINEAR };
    uint32_t toGLFilter(TextureFilter filter);

    enum class TextureWrap { REPEAT, MIRRORED_REPEAT, CLAMP_TO_EDGE, CLAMP_TO_BORDER };
    uint32_t toGLWrap(TextureWrap wrap);
}
