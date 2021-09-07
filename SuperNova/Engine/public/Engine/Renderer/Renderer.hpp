#pragma once

#include <Engine/Renderer/RenderTypes.hpp>

#include <glm/ext/matrix_float4x4.hpp>

#include <vector>
#include <span>


namespace snv
{

class IRendererBackend;


class Renderer
{
public:
    static void Init(GraphicsApi graphicsApi);
    static void Shutdown();

    static void EnableBlend();
    static void EnableDepthTest();

    // TODO(v.matushkin): I think this method shouldn't be In Renderer class, rn it's just a workaround
    static GraphicsApi GetGraphicsApi() { return s_graphicsApi; }

    static void SetBlendFunction(BlendFactor source, BlendFactor destination);
    static void SetClearColor(f32 r, f32 g, f32 b, f32 a);
    static void SetDepthFunction(DepthFunction depthFunction);
    static void SetViewport(i32 x, i32 y, i32 width, i32 height);

    static void Clear(BufferBit bufferBitMask);

    static void RenderFrame(const glm::mat4x4& localToWorld);

    static BufferHandle CreateBuffer(
        std::span<const std::byte>              indexData,
        std::span<const std::byte>              vertexData,
        const std::vector<VertexAttributeDesc>& vertexLayout
    );
    static TextureHandle CreateTexture(const TextureDesc& textureDesc, const ui8* textureData);
    static ShaderHandle  CreateShader(std::span<const char> vertexSource, std::span<const char> fragmentSource);

private:
    static inline GraphicsApi       s_graphicsApi;
    static inline IRendererBackend* s_rendererBackend;
};

} // namespace snv
