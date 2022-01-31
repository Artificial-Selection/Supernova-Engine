#include <Engine/Renderer/OpenGL/GLBuffer.hpp>
#include <Engine/Core/Log.hpp>

#include <glad/glad.h>

#include <utility>


// To surpress MSVC warning C4312: 'type cast': conversion from 'const ui32' to 'void *' of greater size
// It's probably no a good idea to use something like '-Wno-int-to-void-pointer-cast' just for this case
#define UINT_TO_VOID_PTR(ui) (void*)(ui64)(ui)


struct GLVertexAttribute
{
    ui32 Format;
    bool Normalized;
};

static GLVertexAttribute gl_VertexAttribute(snv::VertexAttributeFormat vertexAttributeFormat)
{
    static const GLVertexAttribute gl_VertexAttribute[] = {
        {GL_BYTE,           false},
        {GL_SHORT,          false},
        {GL_INT,            false},
        {GL_UNSIGNED_BYTE,  false},
        {GL_UNSIGNED_SHORT, false},
        {GL_UNSIGNED_INT,   false},
        {GL_BYTE,           true},
        {GL_SHORT,          true},
        {GL_UNSIGNED_BYTE,  true},
        {GL_UNSIGNED_SHORT, true},
        {GL_HALF_FLOAT,     false},
        {GL_FLOAT,          false},
    };

    return gl_VertexAttribute[static_cast<ui8>(vertexAttributeFormat)];
}


namespace snv
{

GLBuffer::GLBuffer() noexcept
    : m_vao(k_InvalidHandle)
    , m_vbo(-1)
    , m_ibo(-1)
{}

GLBuffer::GLBuffer(
    std::span<const std::byte>              indexData,
    std::span<const std::byte>              vertexData,
    const std::vector<VertexAttributeDesc>& vertexLayout
) noexcept
{
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ibo);

    glBindVertexArray(m_vao);
    // TODO(v.matushkin): Use glNamedBufferData()
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexData.size_bytes(), indexData.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertexData.size_bytes(), vertexData.data(), GL_STATIC_DRAW);

    for (const auto& vertexAttribute : vertexLayout)
    {
        const auto attribute = static_cast<ui8>(vertexAttribute.Attribute);
        const auto [format, normalized] = gl_VertexAttribute(vertexAttribute.Format);

        glEnableVertexAttribArray(attribute);
        glVertexAttribPointer(attribute, vertexAttribute.Dimension, format, normalized, 0, UINT_TO_VOID_PTR(vertexAttribute.Offset));
    }
}


GLBuffer::GLBuffer(GLBuffer&& other) noexcept
    : m_vao(std::exchange(other.m_vao, -1))
    , m_vbo(std::exchange(other.m_vbo, -1))
    , m_ibo(std::exchange(other.m_ibo, -1))
{}

GLBuffer& GLBuffer::operator=(GLBuffer&& other) noexcept
{
    // NOTE(v.matushkin): Not sure if this is correct
    m_vao = std::exchange(other.m_vao, -1);
    m_vbo = std::exchange(other.m_vbo, -1);
    m_ibo = std::exchange(other.m_ibo, -1);

    return *this;
}


void GLBuffer::Bind() const
{
    glBindVertexArray(m_vao);
}

} // namespace snv
