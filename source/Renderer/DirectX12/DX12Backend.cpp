#include <Renderer/DirectX12/DX12Backend.hpp>


namespace snv
{

DX12Backend::DX12Backend()
{}


void DX12Backend::EnableBlend()
{}

void DX12Backend::EnableDepthTest()
{}


void DX12Backend::SetBlendFunction(BlendFactor source, BlendFactor destination)
{}

void DX12Backend::SetClearColor(f32 r, f32 g, f32 b, f32 a)
{}

void DX12Backend::SetDepthFunction(DepthFunction depthFunction)
{}

void DX12Backend::SetViewport(i32 x, i32 y, i32 width, i32 height)
{}


void DX12Backend::Clear(BufferBit bufferBitMask)
{}


void DX12Backend::BeginFrame(const glm::mat4x4& localToWorld, const glm::mat4x4& cameraView, const glm::mat4x4& cameraProjection)
{}

void DX12Backend::EndFrame()
{}

void DX12Backend::DrawBuffer(TextureHandle textureHandle, BufferHandle bufferHandle, i32 indexCount, i32 vertexCount)
{}

void DX12Backend::DrawArrays(i32 count)
{}

void DX12Backend::DrawElements(i32 count)
{}


BufferHandle DX12Backend::CreateBuffer(
    std::span<const std::byte>              indexData,
    std::span<const std::byte>              vertexData,
    const std::vector<VertexAttributeDesc>& vertexLayout
)
{
    return BufferHandle();
}

TextureHandle DX12Backend::CreateTexture(const TextureDesc& textureDesc, const ui8* textureData)
{
    return TextureHandle();
}

ShaderHandle DX12Backend::CreateShader(std::span<const char> vertexSource, std::span<const char> fragmentSource)
{
    return ShaderHandle();
}

} // namespace snv
