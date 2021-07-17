#pragma once

#include <Core/Core.hpp>
#include <Renderer/RenderTypes.hpp>

#include <vector>
#include <span>


namespace snv
{

class GLGraphicsBuffer
{
public:
    // NOTE(v.matushkin): Can I make this move only without default constructor?
    // TODO(v.matushkin): Define destructor
    GLGraphicsBuffer() noexcept;

    GLGraphicsBuffer(
        std::span<const std::byte> indexData,
        std::span<const std::byte> vertexData,
        const std::vector<VertexAttributeDescriptor>& vertexLayout
    ) noexcept;

    GLGraphicsBuffer(GLGraphicsBuffer&& other) noexcept;
    GLGraphicsBuffer& operator=(GLGraphicsBuffer&& other) noexcept;

    GLGraphicsBuffer(const GLGraphicsBuffer& other) = delete;
    GLGraphicsBuffer& operator=(const GLGraphicsBuffer& other) = delete;

    void Bind() const;

    [[nodiscard]] GraphicsBufferHandle GetHandle() const { return static_cast<GraphicsBufferHandle>(m_vao); }

private:
    ui32 m_vao;
    ui32 m_vbo;
    ui32 m_ibo;
};

} // namespace snv
