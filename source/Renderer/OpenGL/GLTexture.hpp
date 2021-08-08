#pragma once

#include <Core/Core.hpp>
#include <Renderer/RenderTypes.hpp>


namespace snv
{

class GLTexture
{
public:
    // NOTE(v.matushkin): Can I make this move only without default constructor?
    // TODO(v.matushkin): Define destructor
    GLTexture() noexcept;
    GLTexture(const TextureDesc& textureDesc, const ui8* textureData);

    GLTexture(GLTexture&& other) noexcept;
    GLTexture& operator=(GLTexture&& other) noexcept;

    GLTexture(const GLTexture& other) = delete;
    GLTexture& operator=(const GLTexture& other) = delete;

    [[nodiscard]] TextureHandle GetHandle() const { return static_cast<TextureHandle>(m_textureID); }

    void Bind(ui32 textureUnit) const;

private:
    ui32 m_textureID;
};

} // namespace snv
