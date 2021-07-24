#pragma once

#include <Renderer/RenderTypes.hpp>

#include <glm/ext/matrix_float4x4.hpp>

#include <vector>
#include <span>


namespace snv
{

class IRendererBackend;


class Renderer
{
public:
    static void Init();
    static void Shutdown();

    static void EnableBlend();
    static void EnableDepthTest();

    static void SetBlendFunction(BlendFactor source, BlendFactor destination);
    static void SetClearColor(f32 r, f32 g, f32 b, f32 a);
    static void SetDepthFunction(DepthFunction depthFunction);
    static void SetViewport(i32 x, i32 y, i32 width, i32 height);

    static void Clear(BufferBit bufferBitMask);

    static void StartFrame(
        ShaderHandle shaderHandle, const glm::mat4x4& modelM, const glm::mat4x4& viewM, const glm::mat4x4& projectionM
    );
    static void DrawGraphicsBuffer(
        TextureHandle textureHandle, GraphicsBufferHandle handle, i32 indexCount, i32 vertexCount
    );
    static void DrawArrays(i32 count);
    static void DrawElements(i32 count);

    static GraphicsBufferHandle CreateGraphicsBuffer(
        std::span<const std::byte> indexData,
        std::span<const std::byte> vertexData,
        const std::vector<VertexAttributeDescriptor>& vertexLayout
    );
    static TextureHandle CreateTexture(const TextureDescriptor& textureDescriptor, const ui8* data);
    static ShaderHandle CreateShader(const char* vertexSource, const char* fragmentSource);

private:
    static inline IRendererBackend* s_RendererBackend;
};

} // namespace snv::Rendering
