#include "TeaEngine/Renderer/Texture.h"
#include "TeaEngine/Core/Base.h"
#include "TeaEngine/Core/Log.h"
#include "TeaEngine/Renderer/Image.h"

#include <cstdint>
#include <glad/glad.h>
#include <stb_image.h>
#include <tracy/Tracy.hpp>

namespace Tea {

    std::unordered_map<std::string, Ref<Texture>> TextureLibrary::m_Textures;

    GLenum ImageFormatToOpenGLInternalFormat(ImageFormat format)
    {
        switch(format)
        {
            case ImageFormat::R8: return GL_R8; break;
            case ImageFormat::RG8: return GL_RG8; break;
            case ImageFormat::RGB8: return GL_RGB8; break;
            case ImageFormat::SRGB8: return GL_SRGB8; break;
            case ImageFormat::RGBA8: return GL_RGBA8; break;
            case ImageFormat::SRGBA8: return GL_SRGB8_ALPHA8; break;
            case ImageFormat::RGBA32F: return GL_RGBA32F; break;
            case ImageFormat::DEPTH24STENCIL8: return GL_DEPTH24_STENCIL8; break;
        }
    }

    GLenum ImageFormatToOpenGLFormat(ImageFormat format)
    {
        switch(format)
        {
            case ImageFormat::R8: return GL_RED; break;
            case ImageFormat::RG8: return GL_RG; break;
            case ImageFormat::RGB8: return GL_RGB; break;
            case ImageFormat::SRGB8: return GL_RGB; break;
            case ImageFormat::RGBA8: return GL_RGBA; break;
            case ImageFormat::SRGBA8: return GL_RGBA; break;
            case ImageFormat::RGBA32F: return GL_RGBA; break;
            case ImageFormat::DEPTH24STENCIL8: return GL_DEPTH_STENCIL; break;
        }
    }

    Texture::Texture(const TextureProperties& properties)
        : m_Properties(properties), m_Width(properties.Width), m_Height(properties.Height)
    {
        ZoneScoped;

        Texture(m_Width, m_Height, m_Properties.Format);
    }

    Texture::Texture(uint32_t width, uint32_t height, ImageFormat imageFormat)
        : m_Width(width), m_Height(height), m_Properties({ imageFormat, width, height })
    {
        ZoneScoped;

        GLenum internalFormat = ImageFormatToOpenGLInternalFormat(m_Properties.Format);
        GLenum format = ImageFormatToOpenGLFormat(m_Properties.Format);

        glCreateTextures(GL_TEXTURE_2D, 1, &m_textureID);
        glTextureStorage2D(m_textureID, 1, internalFormat, m_Width, m_Height);

        glTextureParameteri(m_textureID, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(m_textureID, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glTextureParameteri(m_textureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(m_textureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


    }

    Texture::Texture(const std::string& path, bool srgb)
    {
        ZoneScoped;

        m_FilePath = path;

        m_Properties.srgb = srgb;

        int nrComponents;
        stbi_set_flip_vertically_on_load(true);
        unsigned char* m_Data = stbi_load(m_FilePath.c_str(), &m_Width, &m_Height, &nrComponents, 0);
        
        m_Properties.Width = m_Width, m_Properties.Height = m_Height;
        
        if(m_Data)
        {
            switch (nrComponents)
            {
                case 1:
                    m_Properties.Format = ImageFormat::R8;
                break;
                case 3:
                    m_Properties.Format = m_Properties.srgb ? ImageFormat::SRGB8 : ImageFormat::RGB8;
                break;
                case 4:
                    m_Properties.Format = m_Properties.srgb ? ImageFormat::SRGBA8 : ImageFormat::RGBA8;
                break;
            }

            GLenum internalFormat = ImageFormatToOpenGLInternalFormat(m_Properties.Format);
            GLenum format = ImageFormatToOpenGLFormat(m_Properties.Format);

            glCreateTextures(GL_TEXTURE_2D, 1, &m_textureID);
            glTextureStorage2D(m_textureID, 1, internalFormat, m_Width, m_Height);

            glTextureParameteri(m_textureID, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTextureParameteri(m_textureID, GL_TEXTURE_WRAP_T, GL_REPEAT);

            glTextureParameteri(m_textureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTextureParameteri(m_textureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glTextureSubImage2D(m_textureID, 0, 0, 0, m_Width, m_Height, format, GL_UNSIGNED_BYTE, m_Data);

            glGenerateMipmap(GL_TEXTURE_2D);

            stbi_image_free(m_Data);
        }
        else
        {
            TEA_CORE_ERROR("Failed to load texture: {0} (REASON: {1})", m_FilePath, stbi_failure_reason());
            m_textureID = 0; // Set texture ID to 0 to indicate failure
        }
    }

    Texture::~Texture()
    {
        ZoneScoped;

        glDeleteTextures(1, &m_textureID);
    }

    void Texture::Bind(uint32_t slot)
    {
        ZoneScoped;

        glBindTextureUnit(slot, m_textureID);
    }

    void Texture::Resize(uint32_t width, uint32_t height)
    {
        ZoneScoped;

        m_Width = width;
        m_Height = height;

        glDeleteTextures(1, &m_textureID);

        GLenum internalFormat = ImageFormatToOpenGLInternalFormat(m_Properties.Format);
        GLenum format = ImageFormatToOpenGLFormat(m_Properties.Format);

        glCreateTextures(GL_TEXTURE_2D, 1, &m_textureID);
        glTextureStorage2D(m_textureID, 1, internalFormat, m_Width, m_Height);

        glTextureParameteri(m_textureID, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(m_textureID, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glTextureParameteri(m_textureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(m_textureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    }

    void Texture::SetData(void* data, uint32_t size)
    {
        ZoneScoped;

        GLenum format = ImageFormatToOpenGLFormat(m_Properties.Format);
        glTextureSubImage2D(m_textureID, 0, 0, 0, m_Width, m_Height, format, GL_UNSIGNED_BYTE, data);
    }

    Ref<Texture> Texture::Load(const std::string& path, bool srgb)
    {
        std::filesystem::path filePath(path);
        std::string fileName = filePath.filename().string();

        if(TextureLibrary::Exists(fileName))
        {
            return TextureLibrary::Get(fileName);
        }
        else
        {
            Ref<Texture> texture = CreateRef<Texture>(path, srgb);
            TextureLibrary::Add(fileName, texture);
            return texture;
        }
    }

    Ref<Texture> Texture::Create(uint32_t width, uint32_t height, ImageFormat format)
    {
        return CreateRef<Texture>(width, height, format);
    }

}
