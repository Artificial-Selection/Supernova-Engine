#include <Engine/Renderer/OpenGL/GLTexture.hpp>

#include <glad/glad.h>

#include <utility>


// NOTE(v.matushkin): RGBA/BGRA ?
constexpr ui32 gl_TextureFormat[][3] = {
    {GL_R8,                 GL_RED,             GL_UNSIGNED_BYTE},
    {GL_R16,                GL_RED,             GL_UNSIGNED_SHORT},
    {GL_R16F,               GL_RED,             GL_FLOAT},
    {GL_R32F,               GL_RED,             GL_FLOAT},
    {GL_RG8,                GL_RG,              GL_UNSIGNED_BYTE},
    {GL_RG16,               GL_RG,              GL_UNSIGNED_SHORT},
    // {GL_RGB8,               GL_RGB,             GL_UNSIGNED_BYTE},
    // {GL_RGB16,              GL_RGB,             GL_UNSIGNED_SHORT},
    {GL_RGBA8,              GL_RGBA,            GL_UNSIGNED_BYTE},
    {GL_RGBA16F,            GL_RGBA,            GL_FLOAT},
    {GL_DEPTH_COMPONENT16,  GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT},
    // {GL_DEPTH_COMPONENT24,  GL_DEPTH_COMPONENT, GL_UNSIGNED_INT},
    {GL_DEPTH_COMPONENT32,  GL_DEPTH_COMPONENT, GL_UNSIGNED_INT},
    {GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT},
};
constexpr i32 gl_TextureWrapMode[] = {
    GL_CLAMP_TO_EDGE,
    GL_CLAMP_TO_BORDER,
    GL_MIRROR_CLAMP_TO_EDGE,
    GL_MIRRORED_REPEAT,
    GL_REPEAT,
};


namespace snv
{

GLTexture::GLTexture() noexcept
    : m_textureID(k_InvalidHandle)
{}

GLTexture::GLTexture(const TextureDesc& textureDesc, const ui8* textureData)
{
    glGenTextures(1, &m_textureID); // TODO(v.matushkin): glCreateTextures
    glBindTexture(GL_TEXTURE_2D, m_textureID);

    const auto glWrapMode = gl_TextureWrapMode[static_cast<ui8>(textureDesc.WrapMode)];
    glTextureParameteri(m_textureID, GL_TEXTURE_WRAP_S, glWrapMode);
    glTextureParameteri(m_textureID, GL_TEXTURE_WRAP_T, glWrapMode);
    // TODO: Add control for min/mag filtering
    glTextureParameteri(m_textureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(m_textureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    const auto width  = textureDesc.Width;
    const auto height = textureDesc.Height;
    const auto [glInternalFormat, glFormat, glType] = gl_TextureFormat[static_cast<ui8>(textureDesc.Format)];
    glTextureStorage2D(m_textureID, 1, glInternalFormat, width, height);
    glTextureSubImage2D(m_textureID, 0, 0, 0, width, height, glFormat, glType, textureData);
}

GLTexture::GLTexture(GLTexture&& other) noexcept
    : m_textureID(std::exchange(other.m_textureID, k_InvalidHandle))
{}

GLTexture& GLTexture::operator=(GLTexture&& other) noexcept
{
    m_textureID = std::exchange(other.m_textureID, k_InvalidHandle);

    return *this;
}


void GLTexture::Bind(ui32 textureUnit) const
{
    glBindTextureUnit(textureUnit, m_textureID);
}

} // namespace snv
