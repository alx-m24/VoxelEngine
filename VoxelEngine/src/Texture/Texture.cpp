#include "pch.h"
#include "VoxelEngine/Texture/Texture.hpp"
#include "VoxelEngine/Bindable/SmartBinder.hpp"
#include "VoxelEngine/utils/assert.hpp"
#include "VoxelEngine/utils/error.hpp"

#include <glad/glad.h>

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif

#include <stb_image.h>

namespace VoxelEngine {
    Texture::Texture(uint32_t width, uint32_t height, TextureFormat format, const TextureParams& params)
        : m_width(width), m_height(height), m_format(format), m_params(params)
    {
        glGenTextures(1, &m_id);
        V_ASSERT(m_id != 0, "Texture: glGenTextures failed. Is a GL context current?");
        Allocate(nullptr);
    }

    Texture::Texture(const std::string& path, const TextureParams& params)
        : m_format(TextureFormat::RGBA8), m_params(params)
    {
        glGenTextures(1, &m_id);
        V_ASSERT(m_id != 0, "Texture: glGenTextures failed. Is a GL context current?");

        int width = 0, height = 0, channels = 0;
        stbi_set_flip_vertically_on_load(true);
        unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4);

        if (!data) {
            V_LOG_ERROR("Texture: failed to load image: " + path);
            m_width = m_height = 1;
            Allocate(nullptr);
            return;
        }

        m_width  = static_cast<uint32_t>(width);
        m_height = static_cast<uint32_t>(height);
        Allocate(data);

        stbi_image_free(data);
    }

    Texture::~Texture() {
        if (m_id != 0) {
            glDeleteTextures(1, &m_id);
        }
    }

    void Texture::Bind() const {
        glBindTexture(GL_TEXTURE_2D, m_id);
    }

    void Texture::Unbind() const {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void Texture::BindUnit(unsigned int slot) const {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, m_id);
    }

    void Texture::Allocate(const void* pixels) {
        SmartBinder bind(*this);

        const GLFormatInfo info = toGLFormat(m_format);
        glTexImage2D(GL_TEXTURE_2D, 0, static_cast<int>(info.internalFormat),
                static_cast<int>(m_width), static_cast<int>(m_height), 0,
                info.format, info.type, pixels);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<int>(toGLFilter(m_params.minFilter)));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<int>(toGLFilter(m_params.magFilter)));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<int>(toGLWrap(m_params.wrapS)));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<int>(toGLWrap(m_params.wrapT)));

        if (m_params.generateMipmaps) {
            glGenerateMipmap(GL_TEXTURE_2D);
        }
    }
}
