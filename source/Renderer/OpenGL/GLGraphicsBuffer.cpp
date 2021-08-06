#include <Renderer/OpenGL/GLGraphicsBuffer.hpp>
#include <Core/Log.hpp>

#include <glad/glad.h>

#include <utility>


// To surpress MSVC warning C4312: 'type cast': conversion from 'const ui32' to 'void *' of greater size
// It's probably no a good idea to use something like '-Wno-int-to-void-pointer-cast' just for this case
#define UINT_TO_VOID_PTR(ui) (void*)(ui64)(ui)


constexpr ui32 gl_VertexAttributeFormat[] = {
    GL_BYTE ,          // VertexAttributeFormat::Int8
    GL_SHORT,          // VertexAttributeFormat::Int16
    GL_INT,            // VertexAttributeFormat::Int32
    GL_UNSIGNED_BYTE,  // VertexAttributeFormat::UInt8
    GL_UNSIGNED_SHORT, // VertexAttributeFormat::UInt16
    GL_UNSIGNED_INT,   // VertexAttributeFormat::UInt32
    GL_HALF_FLOAT,     // VertexAttributeFormat::Float16
    GL_FLOAT,          // VertexAttributeFormat::Float32
    GL_DOUBLE          // VertexAttributeFormat::Float64
};


namespace snv
{

GLGraphicsBuffer::GLGraphicsBuffer() noexcept
    : m_vao(k_InvalidHandle)
    , m_vbo(-1)
    , m_ibo(-1)
{}

GLGraphicsBuffer::GLGraphicsBuffer(
    std::span<const std::byte> indexData,
    std::span<const std::byte> vertexData,
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
        const auto format    = gl_VertexAttributeFormat[static_cast<ui8>(vertexAttribute.Format)];
        
        glEnableVertexAttribArray(attribute);
        glVertexAttribPointer(attribute, vertexAttribute.Dimension, format, GL_FALSE, 0, UINT_TO_VOID_PTR(vertexAttribute.Offset));
    }
}


GLGraphicsBuffer::GLGraphicsBuffer(GLGraphicsBuffer&& other) noexcept
    : m_vao(std::exchange(other.m_vao, -1))
    , m_vbo(std::exchange(other.m_vbo, -1))
    , m_ibo(std::exchange(other.m_ibo, -1))
{}

GLGraphicsBuffer& GLGraphicsBuffer::operator=(GLGraphicsBuffer&& other) noexcept
{
    // NOTE(v.matushkin): Not sure if this is correct
    m_vao = std::exchange(other.m_vao, -1);
    m_vbo = std::exchange(other.m_vbo, -1);
    m_ibo = std::exchange(other.m_ibo, -1);

    return *this;
}


void GLGraphicsBuffer::Bind() const
{
    glBindVertexArray(m_vao);
}

} // namespace snv
