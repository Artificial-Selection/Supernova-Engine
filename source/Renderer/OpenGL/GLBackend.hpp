#pragma once

#include <Core/Core.hpp>
#include <Renderer/IRendererBackend.hpp>
#include <Renderer/OpenGL/GLGraphicsBuffer.hpp>
#include <Renderer/OpenGL/GLTexture.hpp>

#include <unordered_map>


namespace snv
{

struct VertexAttributeDescriptor;


class GLBackend final : public IRendererBackend
{
public:
    GLBackend();
    ~GLBackend() override = default;

    void EnableBlend() override;
    void EnableDepthTest() override;

    void SetBlendFunction(BlendFactor source, BlendFactor destination) override;
    void SetClearColor(f32 r, f32 g, f32 b, f32 a) override;
    void SetDepthFunction(DepthFunction depthFunction) override;
    void SetViewport(i32 x, i32 y, i32 width, i32 height) override;

    void Clear(BufferBit bufferBitMask) override;

    void DrawGraphicsBuffer(GraphicsBufferHandle handle, TextureHandle textureHandle, i32 indexCount, i32 vertexCount) override;
    void DrawArrays(i32 count) override;
    void DrawElements(i32 count) override;

    GraphicsBufferHandle CreateGraphicsBuffer(
        std::span<const std::byte> indexData,
        std::span<const std::byte> vertexData,
        const std::vector<VertexAttributeDescriptor>& vertexLayout
    ) override;
    TextureHandle CreateTexture(const TextureDescriptor& textureDescriptor, const ui8* data) override;

private:
    std::unordered_map<GraphicsBufferHandle, GLGraphicsBuffer> m_graphicsBuffers;
    std::unordered_map<TextureHandle,        GLTexture>        m_textures;
};

} // namespace snv
