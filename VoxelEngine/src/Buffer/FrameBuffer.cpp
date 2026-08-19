#include "pch.h"
#include "VoxelEngine/Buffer/FrameBuffer.hpp"
#include "VoxelEngine/Texture/Texture.hpp"
#include "VoxelEngine/Bindable/SmartBinder.hpp"
#include "VoxelEngine/utils/assert.hpp"

#include <glad/glad.h>
#include <algorithm>
#include <vector>

namespace VoxelEngine {
    Framebuffer::Framebuffer() {
        glGenFramebuffers(1, &m_id);
        V_ASSERT(m_id != 0, "Framebuffer: glGenFramebuffers failed. Is a GL context current?");
    }

    Framebuffer::~Framebuffer() {
        if (m_id != 0) {
            glDeleteFramebuffers(1, &m_id);
        }
    }

    void Framebuffer::BindDefault() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void Framebuffer::Bind() const {
        glBindFramebuffer(GL_FRAMEBUFFER, m_id);
    }

    void Framebuffer::Unbind() const {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void Framebuffer::AttachColor(Texture& texture, unsigned int slot) {
        SmartBinder bind(*this);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + slot, GL_TEXTURE_2D, texture.GetID(), 0);

        m_colorAttachmentCount = std::max(m_colorAttachmentCount, slot + 1);
        m_colorAttachments.push_back({ &texture, slot });

        std::vector<unsigned int> drawBuffers(m_colorAttachmentCount);
        for (unsigned int i = 0; i < m_colorAttachmentCount; i++) {
            drawBuffers[i] = GL_COLOR_ATTACHMENT0 + i;
        }
        glDrawBuffers(static_cast<int>(drawBuffers.size()), drawBuffers.data());
    }

    void Framebuffer::AttachDepth(Texture& texture) {
        m_depthAttachment = &texture;
        SmartBinder bind(*this);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture.GetID(), 0);
    }

    void Framebuffer::AttachDepthStencil(Texture& texture) {
        m_depthAttachment = &texture;
        SmartBinder bind(*this);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, texture.GetID(), 0);
    }

    bool Framebuffer::IsComplete() const {
        SmartBinder bind(*this);
        return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    }

    void Framebuffer::Resize(uint32_t width, uint32_t height) {
        for (auto& attachment : m_colorAttachments) {
            attachment.texture->resize(width, height);
        }
        if (m_depthAttachment) { m_depthAttachment->resize(width, height); }
    }
}
