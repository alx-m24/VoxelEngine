// Framebuffer.hpp
#pragma once

#include "VoxelEngine/Bindable/Bindable.hpp"
#include <vector>

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

            void AttachColor(Texture& texture, unsigned int slot = 0);
            void AttachDepth(Texture& texture);
            void AttachDepthStencil(Texture& texture);

            void Resize(uint32_t width, uint32_t height);

            bool IsComplete() const;

            static void BindDefault();

        private:
            struct ColorAttachment {
                Texture*     texture;
                unsigned int slot;
            };

            unsigned int m_id = 0;
            unsigned int m_colorAttachmentCount = 0;
            std::vector<ColorAttachment> m_colorAttachments;
            Texture* m_depthAttachment = nullptr;
    };
}
