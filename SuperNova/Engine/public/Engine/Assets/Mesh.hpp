#pragma once

#include <Engine/Core/Core.hpp>
#include <Engine/Renderer/RenderTypes.hpp>

#include <memory>
#include <vector>


namespace snv
{

class Mesh
{
public:
    Mesh(
        i32 indexCount, std::unique_ptr<ui32[]>&& indexData,
        i32 vertexCount, std::unique_ptr<ui8[]>&& vertexData,
        const std::vector<VertexAttributeDesc>& vertexLayout
    );

    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    Mesh(const Mesh& other) = delete;
    Mesh& operator=(const Mesh& other) = delete;

    [[nodiscard]] i32 GetIndexCount()  const { return m_indexCount; }
    [[nodiscard]] i32 GetVertexCount() const { return m_vertexCount; }
    [[nodiscard]] BufferHandle GetHandle() const { return m_bufferHandle; }

private:
    std::unique_ptr<ui32[]> m_indexData;
    std::unique_ptr<ui8[]>  m_vertexData;

    i32                     m_indexCount;
    i32                     m_vertexCount;

    BufferHandle            m_bufferHandle;
};

} // namespace snv
