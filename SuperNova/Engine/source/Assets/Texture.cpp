#include <Engine/Assets/Texture.hpp>
#include <Engine/Renderer/Renderer.hpp>

#include <utility>


namespace snv
{

Texture::Texture(const TextureDesc& textureDesc, std::unique_ptr<ui8[]>&& textureData)
    : m_textureData(std::move(textureData))
    , m_textureHandle(Renderer::CreateTexture(textureDesc, m_textureData.get()))
{}

Texture::Texture(Texture&& other) noexcept
    : m_textureData(std::exchange(other.m_textureData, nullptr))
    , m_textureHandle(std::exchange(other.m_textureHandle, TextureHandle::InvalidHandle))
{}

Texture& Texture::operator=(Texture&& other) noexcept
{
    m_textureData   = std::exchange(other.m_textureData, nullptr);
    m_textureHandle = std::exchange(other.m_textureHandle, TextureHandle::InvalidHandle);

    return *this;
}


static constexpr TextureDesc s_DefaultTextureDesc = {
    .Width    = 4,
    .Height   = 4,
    .Format   = TextureFormat::RGBA8,
    .WrapMode = TextureWrapMode::Repeat,
};

// NOTE(v.matushkin): Not sure about this methods
//   May be this textures needs to be registered in AssetDatabase when I have Asset GUID's
//   May be just store them on disk?

std::shared_ptr<Texture> Texture::GetBlackTexture()
{
    static constexpr ui8 blackTextureData[4 * 4 * 4] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    };
    auto blackTextureDataPtr = std::make_unique<ui8[]>(4 * 4 * 4);
    std::memcpy(blackTextureDataPtr.get(), blackTextureData, 4 * 4 * 4);

    static auto blackTexture = std::make_shared<Texture>(s_DefaultTextureDesc, std::move(blackTextureDataPtr));

    return blackTexture;
}

std::shared_ptr<Texture> Texture::GetWhiteTexture()
{
    static constexpr ui8 whiteTextureData[4 * 4 * 4] = {
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    };
    auto whiteTextureDataPtr = std::make_unique<ui8[]>(4 * 4 * 4);
    std::memcpy(whiteTextureDataPtr.get(), whiteTextureData, 4 * 4 * 4);

    static auto whiteTexture = std::make_shared<Texture>(s_DefaultTextureDesc, std::move(whiteTextureDataPtr));

    return whiteTexture;
}

std::shared_ptr<Texture> Texture::GetNormalTexture()
{
    static constexpr ui8 normalTextureData[4 * 4 * 4] = {
        127, 127, 255, 255, 127, 127, 255, 255, 127, 127, 255, 255, 127, 127, 255, 255,
        127, 127, 255, 255, 127, 127, 255, 255, 127, 127, 255, 255, 127, 127, 255, 255,
        127, 127, 255, 255, 127, 127, 255, 255, 127, 127, 255, 255, 127, 127, 255, 255,
        127, 127, 255, 255, 127, 127, 255, 255, 127, 127, 255, 255, 127, 127, 255, 255,
    };
    auto normalTextureDataPtr = std::make_unique<ui8[]>(4 * 4 * 4);
    std::memcpy(normalTextureDataPtr.get(), normalTextureData, 4 * 4 * 4);

    static auto normalTexture = std::make_shared<Texture>(s_DefaultTextureDesc, std::move(normalTextureDataPtr));

    return normalTexture;
}

} // namespace snv
