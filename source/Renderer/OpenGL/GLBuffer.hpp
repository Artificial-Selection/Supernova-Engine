#pragma once

#include <Core/Core.hpp>
#include <Renderer/RenderTypes.hpp>

#include <vector>
#include <span>


namespace snv
{

class GLBuffer
{
public:
    // NOTE(v.matushkin): Can I make this move only without default constructor?
    // TODO(v.matushkin): Define destructor
    GLBuffer() noexcept;

    GLBuffer(
        std::span<const std::byte>              indexData,
        std::span<const std::byte>              vertexData,
        const std::vector<VertexAttributeDesc>& vertexLayout
    ) noexcept;

    GLBuffer(GLBuffer&& other) noexcept;
    GLBuffer& operator=(GLBuffer&& other) noexcept;

    GLBuffer(const GLBuffer& other) = delete;
    GLBuffer& operator=(const GLBuffer& other) = delete;

    [[nodiscard]] BufferHandle GetHandle() const { return static_cast<BufferHandle>(m_vao); }

    void Bind() const;

private:
    ui32 m_vao;
    ui32 m_vbo;
    ui32 m_ibo;
};

} // namespace snv
