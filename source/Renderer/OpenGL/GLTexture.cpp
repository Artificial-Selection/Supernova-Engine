#include <Renderer/OpenGL/GLTexture.hpp>

#include <glad/glad.h>

#include <utility>


// NOTE(v.matushkin): RGBA/BGRA ?
constexpr ui32 gl_formatTable[][3] = {
    {GL_R8,                 GL_RED,             GL_UNSIGNED_BYTE},
    {GL_R16,                GL_RED,             GL_UNSIGNED_SHORT},
    {GL_R16F,               GL_RED,             GL_FLOAT},
    {GL_R32F,               GL_RED,             GL_FLOAT},
    {GL_RG8,                GL_RG,              GL_UNSIGNED_BYTE},
    {GL_RG16,               GL_RG,              GL_UNSIGNED_SHORT},
    {GL_RGB8,               GL_RGB,             GL_UNSIGNED_BYTE},
    {GL_RGB16,              GL_RGB,             GL_UNSIGNED_SHORT},
    {GL_RGBA8,              GL_RGBA,            GL_UNSIGNED_BYTE},
    {GL_RGBA16F,            GL_RGBA,            GL_FLOAT},
    {GL_DEPTH_COMPONENT16,  GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT},
    {GL_DEPTH_COMPONENT24,  GL_DEPTH_COMPONENT, GL_UNSIGNED_INT},
    {GL_DEPTH_COMPONENT32,  GL_DEPTH_COMPONENT, GL_UNSIGNED_INT},
    {GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT},
};
constexpr i32 gl_wrapTable[] = {
    GL_CLAMP_TO_EDGE,
    GL_CLAMP_TO_BORDER,
    GL_MIRRORED_REPEAT,
    GL_REPEAT,
};


namespace snv
{

GLTexture::GLTexture() noexcept
    : m_textureID(k_InvalidHandle)
{}

GLTexture::GLTexture(const TextureDescriptor& textureDescriptor, const ui8* data)
{
    glGenTextures(1, &m_textureID);
    glBindTexture(GL_TEXTURE_2D, m_textureID);

    const auto glWrapMode = gl_wrapTable[static_cast<ui8>(textureDescriptor.WrapMode)];
    glTextureParameteri(m_textureID, GL_TEXTURE_WRAP_S, glWrapMode);
    glTextureParameteri(m_textureID, GL_TEXTURE_WRAP_T, glWrapMode);

    const auto width  = textureDescriptor.Width;
    const auto height = textureDescriptor.Height;
    const auto[glInternalFormat, glFormat, glType] = gl_formatTable[static_cast<ui8>(textureDescriptor.GraphicsFormat)];
    glTextureStorage2D(m_textureID, 1, glInternalFormat, width, height);
    glTextureSubImage2D(m_textureID, 0, 0, 0, width, height, glFormat, glType, data);
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
