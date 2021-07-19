#pragma once

#include <Renderer/RenderTypes.hpp>


namespace snv
{

class Material
{
public:
    Material(const TextureDescriptor& textureDescriptor, const ui8* data);

    [[nodiscard]] TextureHandle GetTextureHandle() const { return m_textureHandle; }

private:
    TextureHandle m_textureHandle;
};

} // namespace snv
