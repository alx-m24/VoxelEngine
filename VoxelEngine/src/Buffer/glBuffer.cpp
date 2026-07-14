#include "pch.h"
#include "VoxelEngine/Buffer/glBuffer.hpp"
#include "VoxelEngine/Bindable/SmartBinder.hpp"
#include "VoxelEngine/utils/assert.hpp"
#include <glad/glad.h>

namespace VoxelEngine {

    GLBuffer::GLBuffer(BufferTarget target) : m_target(target) {
        if (m_id == 0) {
            glGenBuffers(1, &m_id);
            V_ASSERT(m_id != 0, "GLBuffer: glGenBuffers failed — is a GL context current?");
        }
    }

    GLBuffer::~GLBuffer() {
        if (m_id != 0) {
            glDeleteBuffers(1, &m_id); // no-op on id 0 anyway, but skip the call entirely if never created
        }
    }

    void GLBuffer::Bind() const {
        glBindBuffer(toGLTarget(m_target), m_id);
    }

    void GLBuffer::Unbind() const {
        glBindBuffer(toGLTarget(m_target), 0);
    }

    void GLBuffer::SetData(const void* data, size_t size, BufferUsage usage) {
        SmartBinder bind(*this); // triggers EnsureCreated via Bind()
        glBufferData(toGLTarget(m_target), size, data, toGLUsage(usage));
    }
}
