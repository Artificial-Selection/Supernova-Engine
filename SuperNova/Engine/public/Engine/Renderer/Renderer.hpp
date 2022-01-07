#pragma once

#include <Engine/Renderer/IImGuiRenderContext.hpp> // NOTE(v.matushkin): Forward declare?
#include <Engine/Renderer/RenderTypes.hpp>

#include <glm/ext/matrix_float4x4.hpp>

#include <memory>
#include <span>
#include <vector>


namespace snv
{

class IRendererBackend;
class RenderGraph;


class Renderer
{
public:
    static void Init();
    static void Shutdown();

    static void EnableBlend();
    static void EnableDepthTest();

    // For ImGui
    static [[nodiscard]] void*            GetNativeRenderTexture(RenderTextureHandle renderTextureHandle);
    static [[nodiscard]] RenderPassHandle GetSwapchainRenderPass();

    static void SetBlendFunction(BlendMode source, BlendMode destination);
    static void SetClearColor(f32 r, f32 g, f32 b, f32 a);
    static void SetDepthFunction(DepthCompareFunction depthCompareFunction);
    static void SetViewport(i32 x, i32 y, i32 width, i32 height);

    static void Clear(BufferBit bufferBitMask);

    static void RenderFrame(const glm::mat4x4& localToWorld);

    static [[nodiscard]] IImGuiRenderContext* CreateImGuiRenderContext();

    static [[nodiscard]] RenderTextureHandle CreateRenderTexture(const RenderTextureDesc& renderTextureDesc);
    static [[nodiscard]] RenderPassHandle    CreateRenderPass(const RenderPassDesc& renderPassDesc);

    static [[nodiscard]] BufferHandle  CreateBuffer(
        std::span<const std::byte>              indexData,
        std::span<const std::byte>              vertexData,
        const std::vector<VertexAttributeDesc>& vertexLayout
    );
    static [[nodiscard]] TextureHandle CreateTexture(const TextureDesc& textureDesc, const ui8* textureData);
    static [[nodiscard]] ShaderHandle  CreateShader(const ShaderDesc& shaderDesc);

private:
    static inline IRendererBackend*   s_rendererBackend;
    static inline RenderGraph*        s_renderGraph;
};

} // namespace snv
