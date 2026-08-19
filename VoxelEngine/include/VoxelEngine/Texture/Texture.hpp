#pragma once

#include <cstdint>
#include <string>

#include "VoxelEngine/Bindable/Bindable.hpp"
#include "VoxelEngine/utils/types.hpp"

namespace VoxelEngine {
    struct TextureParams {
        TextureFilter minFilter      = TextureFilter::LINEAR;
        TextureFilter magFilter      = TextureFilter::LINEAR;
        TextureWrap   wrapS          = TextureWrap::CLAMP_TO_EDGE;
        TextureWrap   wrapT          = TextureWrap::CLAMP_TO_EDGE;
        bool          generateMipmaps = false;
    };

    class Texture : public Bindable {
        public:
            Texture(uint32_t width, uint32_t height, TextureFormat format, const TextureParams& params = {});

            explicit Texture(const std::string& path, const TextureParams& params = {});

            ~Texture() override;

            Texture(const Texture&) = delete;
            Texture& operator=(const Texture&) = delete;

            void Bind() const override;
            void Unbind() const override;

            // Binds to a specific texture unit (glActiveTexture + Bind), for sampler use in shaders
            void BindUnit(unsigned int slot) const;

            uint32_t      GetID() const     { return m_id; }
            uint32_t      GetWidth() const  { return m_width; }
            uint32_t      GetHeight() const { return m_height; }
            TextureFormat GetFormat() const { return m_format; }

            void resize(uint32_t width, uint32_t height);

        private:
            void Allocate(const void* pixels);

        private:
            uint32_t      m_id = 0;
            uint32_t      m_width = 0;
            uint32_t      m_height = 0;
            TextureFormat m_format;
            TextureParams m_params;
    };
}
