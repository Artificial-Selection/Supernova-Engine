#include <Assets/Texture.hpp>
#include <Renderer/Renderer.hpp>

#include <utility>


namespace snv
{

Texture::Texture(const TextureDescriptor& textureDescriptor, std::unique_ptr<ui8[]>&& textureData)
    : m_textureData(std::move(textureData))
{
    m_textureHandle = snv::Renderer::CreateTexture(textureDescriptor, m_textureData.get());
}

Texture::Texture(Texture&& other) noexcept
    : m_textureHandle(std::exchange(other.m_textureHandle, TextureHandle::InvalidHandle))
{}

Texture& Texture::operator=(Texture&& other) noexcept
{
    m_textureHandle = std::exchange(other.m_textureHandle, TextureHandle::InvalidHandle);

    return *this;
}

} // namespace snv
