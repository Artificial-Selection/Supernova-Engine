#include <Engine/Assets/Mesh.hpp>
#include <Engine/Renderer/Renderer.hpp>

#include <span>
#include <utility>


namespace snv
{

Mesh::Mesh(
    i32 indexCount, std::unique_ptr<ui32[]>&& indexData,
    i32 vertexCount, std::unique_ptr<ui8[]>&& vertexData,
    const std::vector<VertexAttributeDesc>& vertexLayout
)
    : m_indexData(std::move(indexData))
    , m_vertexData(std::move(vertexData))
    , m_indexCount(indexCount)
    , m_vertexCount(vertexCount)
{
    i32 vertexDataElements = 0;
    for (const auto& vertexAttribute : vertexLayout)
    {
        vertexDataElements += vertexCount * (vertexAttribute.Dimension * sizeof(f32));
    }

    m_bufferHandle = Renderer::CreateBuffer(
        std::as_bytes(std::span(m_indexData.get(), m_indexCount)),
        std::as_bytes(std::span(m_vertexData.get(), vertexDataElements)),
        vertexLayout
    );
}

Mesh::Mesh(Mesh&& other) noexcept
    : m_indexData(std::exchange(other.m_indexData, nullptr))
    , m_vertexData(std::exchange(other.m_vertexData, nullptr))
    , m_indexCount(std::exchange(other.m_indexCount, -1))
    , m_vertexCount(std::exchange(other.m_vertexCount, -1))
    , m_bufferHandle(std::exchange(other.m_bufferHandle, BufferHandle::InvalidHandle))
{}

Mesh& Mesh::operator=(Mesh&& other) noexcept
{
    m_indexData  = std::exchange(other.m_indexData, nullptr);
    m_vertexData = std::exchange(other.m_vertexData, nullptr);
    m_indexCount  = std::exchange(other.m_indexCount, -1);
    m_vertexCount = std::exchange(other.m_vertexCount, -1);
    m_bufferHandle = std::exchange(other.m_bufferHandle, BufferHandle::InvalidHandle);

    return *this;
}

} // namespace snv
