#include <Assets/Texture.hpp>
#include <Core/Assert.hpp>
#include <Renderer/Renderer.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <string>
#include <utility>


namespace snv
{

Texture::Texture(const TextureDescriptor& textureDescriptor, std::unique_ptr<ui8>&& textureData)
    : m_textureData(std::move(textureData))
{
    m_textureHandle = snv::Renderer::CreateTexture(textureDescriptor, m_textureData.get());
}

Texture::~Texture()
{
    // TODO(v.matushkin): Thanks to fuckin stb there is a memory leak
    //stbi_image_free(m_textureData.get());
}

Texture::Texture(Texture&& other) noexcept
    : m_textureHandle(std::exchange(other.m_textureHandle, TextureHandle::InvalidHandle))
{}

Texture& Texture::operator=(Texture&& other) noexcept
{
    m_textureHandle = std::exchange(other.m_textureHandle, TextureHandle::InvalidHandle);

    return *this;
}


Texture Texture::LoadAsset(const char* texturePath)
{
    // TODO(v.matushkin): Asset class shouldn't handle path adjusting
    std::string fullPath = "../../assets/models/Sponza/" + std::string(texturePath);

    i32 width, height, numComponents;
    // TODO(v.matushkin): Assert data != null
    ui8* data = stbi_load(fullPath.c_str(), &width, &height, &numComponents, 0);
    SNV_ASSERT(data != nullptr, "Error while loading texture");

    // NOTE(v.matushkin): Idk why the fuck make_unique doesn't work
    //std::unique_ptr<ui8> textureData = std::make_unique<ui8(data);
    std::unique_ptr<ui8> textureData(data);

    TextureDescriptor textureDescriptor{
        .Width          = width,
        .Height         = height,
        .GraphicsFormat = TextureGraphicsFormat::RGB8,
        .WrapMode       = TextureWrapMode::ClampToBorder
    };

    return Texture(textureDescriptor, std::move(textureData));
}

} // namespace snv
