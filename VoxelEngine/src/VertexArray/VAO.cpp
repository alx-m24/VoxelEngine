#include "pch.h"
#include "VoxelEngine/Buffer/VAO.hpp"

#include "VoxelEngine/Bindable/SmartBinder.hpp"

namespace VoxelEngine {
    VertexArray::VertexArray()  { 
        glGenVertexArrays(1, &m_id);
    }
    VertexArray::~VertexArray() { 
        glDeleteVertexArrays(1, &m_id);
    }
    
    void VertexArray::Bind() const { 
        glBindVertexArray(m_id);
    }

    void VertexArray::Unbind() const { 
        glBindVertexArray(0);
    }
    
    void VertexArray::AddVertexBuffer(const VertexBuffer& vbo, const VertexAttribute& attr) {
        SmartBinder bindVao(*this);
        vbo.Bind();
        glEnableVertexAttribArray(attr.index);
        glVertexAttribPointer(attr.index, attr.count, toGLType(attr.type), 
                attr.normalized, attr.stride, (const void*)attr.offset);
    }

    void VertexArray::SetElementBuffer(const ElementBuffer& ebo) {
        SmartBinder bindVao(*this);
        ebo.Bind();
    }
}
