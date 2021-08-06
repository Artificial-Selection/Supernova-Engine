#pragma once

#include <Core/Core.hpp>
#include <Renderer/RenderTypes.hpp>

#include <glm/ext/matrix_float4x4.hpp>

#include <vector>
#include <span>


namespace snv
{

class IRendererBackend
{
public:
    virtual ~IRendererBackend() {}

    virtual void EnableBlend() = 0;
    virtual void EnableDepthTest() = 0;

    virtual void SetBlendFunction(BlendFactor source, BlendFactor destination) = 0;
    virtual void SetClearColor(f32 r, f32 g, f32 b, f32 a) = 0;
    virtual void SetDepthFunction(DepthFunction depthFunction) = 0;
    virtual void SetViewport(i32 x, i32 y, i32 width, i32 height) = 0;

    virtual void Clear(BufferBit bufferBitMask) = 0;

    // TODO(v.matushkin): Remove, temporary method
    virtual void BeginFrame(const glm::mat4x4& localToWorld, const glm::mat4x4& cameraView, const glm::mat4x4& cameraProjection) = 0;
    virtual void EndFrame() = 0;
    // NOTE(v.matushkin): Questionable method
    virtual void DrawGraphicsBuffer(
        TextureHandle textureHandle, GraphicsBufferHandle handle, i32 indexCount, i32 vertexCount
    ) = 0;
    virtual void DrawArrays(i32 count) = 0;
    virtual void DrawElements(i32 count) = 0;

    virtual GraphicsBufferHandle CreateGraphicsBuffer(
        std::span<const std::byte> indexData,
        std::span<const std::byte> vertexData,
        const std::vector<VertexAttributeDescriptor>& vertexLayout
    ) = 0;
    virtual TextureHandle CreateTexture(const TextureDescriptor& textureDescriptor, const ui8* data) = 0;
    virtual ShaderHandle  CreateShader(std::span<const char> vertexSource, std::span<const char> fragmentSource) = 0;
};

} // namespace snv
