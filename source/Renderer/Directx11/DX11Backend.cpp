#include <Renderer/Directx11/DX11Backend.hpp>


namespace snv
{

DX11Backend::DX11Backend()
{
}


void DX11Backend::EnableBlend()
{
}

void DX11Backend::EnableDepthTest()
{
}


void DX11Backend::SetBlendFunction(BlendFactor source, BlendFactor destination)
{
}

void DX11Backend::SetClearColor(f32 r, f32 g, f32 b, f32 a)
{
}

void DX11Backend::SetDepthFunction(DepthFunction depthFunction)
{
}

void DX11Backend::SetViewport(i32 x, i32 y, i32 width, i32 height)
{
}


void DX11Backend::Clear(BufferBit bufferBitMask)
{
}


void DX11Backend::BeginFrame(const glm::mat4x4& localToWorld, const glm::mat4x4& cameraView, const glm::mat4x4& cameraProjection)
{
}

void DX11Backend::EndFrame()
{
}

void DX11Backend::DrawGraphicsBuffer(TextureHandle textureHandle, GraphicsBufferHandle handle, i32 indexCount, i32 vertexCount)
{
}

void DX11Backend::DrawArrays(i32 count)
{
}

void DX11Backend::DrawElements(i32 count)
{
}


GraphicsBufferHandle DX11Backend::CreateGraphicsBuffer(
    std::span<const std::byte> indexData, std::span<const std::byte> vertexData,
    const std::vector<VertexAttributeDescriptor>& vertexLayout)
{
    return GraphicsBufferHandle();
}

TextureHandle DX11Backend::CreateTexture(const TextureDescriptor& textureDescriptor, const ui8* data)
{
    return TextureHandle();
}

ShaderHandle DX11Backend::CreateShader(const char* vertexSource, const char* fragmentSource)
{
    return ShaderHandle();
}

} // namespace snv
