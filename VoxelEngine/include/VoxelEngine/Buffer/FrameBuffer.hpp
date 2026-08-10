#pragma once

#include "VoxelEngine/Bindable/Bindable.hpp"

namespace VoxelEngine {
    class Texture;

    class Framebuffer : public Bindable {
        public:
            Framebuffer();
            ~Framebuffer() override;

            Framebuffer(const Framebuffer&) = delete;
            Framebuffer& operator=(const Framebuffer&) = delete;

            void Bind() const override;
            void Unbind() const override;

            void AttachColor(const Texture& texture, unsigned int slot = 0);
            void AttachDepth(const Texture& texture);
            void AttachDepthStencil(const Texture& texture);

            bool IsComplete() const;

        private:
            unsigned int m_id = 0;
            unsigned int m_colorAttachmentCount = 0;
    };
}
