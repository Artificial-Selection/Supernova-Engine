#pragma once

#include <Renderer/RenderTypes.hpp>

#include <memory>


namespace snv
{

class Texture
{
public:
    Texture(const TextureDesc& textureDesc, std::unique_ptr<ui8[]>&& textureData);

    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    Texture(const Texture& other) = delete;
    Texture& operator=(const Texture& other) = delete;

    [[nodiscard]] TextureHandle GetTextureHandle() const { return m_textureHandle; }

    [[nodiscard]] static std::shared_ptr<Texture> GetBlackTexture();
    [[nodiscard]] static std::shared_ptr<Texture> GetWhiteTexture();
    [[nodiscard]] static std::shared_ptr<Texture> GetNormalTexture();

private:
    std::unique_ptr<ui8[]> m_textureData;

    TextureHandle          m_textureHandle;
};

} // namespace snv
