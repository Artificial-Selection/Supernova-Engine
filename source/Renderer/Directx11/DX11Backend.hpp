#pragma once

#include <Renderer/IRendererBackend.hpp>


namespace snv
{

class DX11Backend final : public IRendererBackend
{
public:
    DX11Backend();

    void EnableBlend() override;
    void EnableDepthTest() override;

    void SetBlendFunction(BlendFactor source, BlendFactor destination) override;
    void SetClearColor(f32 r, f32 g, f32 b, f32 a) override;
    void SetDepthFunction(DepthFunction depthFunction) override;
    void SetViewport(i32 x, i32 y, i32 width, i32 height) override;

    void Clear(BufferBit bufferBitMask) override;

    void BeginFrame(const glm::mat4x4& localToWorld, const glm::mat4x4& cameraView, const glm::mat4x4& cameraProjection) override;
    void EndFrame() override;
    void DrawGraphicsBuffer(TextureHandle textureHandle, GraphicsBufferHandle handle, i32 indexCount, i32 vertexCount) override;
    void DrawArrays(i32 count) override;
    void DrawElements(i32 count) override;

    GraphicsBufferHandle CreateGraphicsBuffer(
        std::span<const std::byte> indexData, std::span<const std::byte> vertexData,
        const std::vector<VertexAttributeDescriptor>& vertexLayout) override;
    TextureHandle CreateTexture(const TextureDescriptor& textureDescriptor, const ui8* data) override;
    ShaderHandle  CreateShader(const char* vertexSource, const char* fragmentSource) override;

private:

};

} // namespace snv
