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
    GLTexture(const TextureDescriptor& textureDescriptor, const ui8* data);

    GLTexture(GLTexture&& other) noexcept;
    GLTexture& operator=(GLTexture&& other) noexcept;

    GLTexture(const GLTexture& other) = delete;
    GLTexture& operator=(const GLTexture& other) = delete;

    void Bind(ui32 textureUnit) const;

    [[nodiscard]] TextureHandle GetHandle() const { return static_cast<TextureHandle>(m_textureID); }

private:
    ui32 m_textureID;
};

} // namespace snv
