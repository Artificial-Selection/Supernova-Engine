#include <Renderer/Renderer.hpp>
#include <Renderer/IRendererBackend.hpp>
#include <Renderer/OpenGL/GLBackend.hpp>


namespace snv
{

IRendererBackend* Renderer::s_RendererBackend;


void Renderer::Init()
{
    s_RendererBackend = new GLBackend();
}

void Renderer::Shutdown()
{
    delete s_RendererBackend;
}


void Renderer::EnableBlend()
{
    s_RendererBackend->EnableBlend();
}

void Renderer::EnableDepthTest()
{
    s_RendererBackend->EnableDepthTest();
}


void Renderer::SetBlendFunction(BlendFactor source, BlendFactor destination)
{
    s_RendererBackend->SetBlendFunction(source, destination);
}

void Renderer::SetClearColor(f32 r, f32 g, f32 b, f32 a)
{
    s_RendererBackend->SetClearColor(r, g, b, a);
}

void Renderer::SetDepthFunction(DepthFunction depthFunction)
{
    s_RendererBackend->SetDepthFunction(depthFunction);
}

void Renderer::SetViewport(i32 x, i32 y, i32 width, i32 height)
{
    s_RendererBackend->SetViewport(x, y, width, height);
}

void Renderer::Clear(BufferBit bufferBitMask)
{
    s_RendererBackend->Clear(bufferBitMask);
}


void Renderer::DrawGraphicsBuffer(GraphicsBufferHandle handle, TextureHandle textureHandle, i32 indexCount, i32 vertexCount)
{
    s_RendererBackend->DrawGraphicsBuffer(handle, textureHandle, indexCount, vertexCount);
}

void Renderer::DrawArrays(i32 count)
{
    s_RendererBackend->DrawArrays(count);
}

void Renderer::DrawElements(i32 count)
{
    s_RendererBackend->DrawElements(count);
}


GraphicsBufferHandle Renderer::CreateGraphicsBuffer(
    std::span<const std::byte> indexData,
    std::span<const std::byte> vertexData,
    const std::vector<VertexAttributeDescriptor>& vertexLayout
)
{
    return s_RendererBackend->CreateGraphicsBuffer(indexData, vertexData, vertexLayout);
}

TextureHandle Renderer::CreateTexture(const TextureDescriptor& textureDescriptor, const ui8* data)
{
    return s_RendererBackend->CreateTexture(textureDescriptor, data);
}

}
