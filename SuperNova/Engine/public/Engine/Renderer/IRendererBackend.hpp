#pragma once

#include <Engine/Core/Core.hpp>
#include <Engine/Renderer/RenderTypes.hpp>

#include <glm/ext/matrix_float4x4.hpp>

#include <vector>
#include <span>


namespace snv
{

struct IImGuiRenderContext;


class IRendererBackend
{
public:
    virtual ~IRendererBackend() = default;

    virtual void EnableBlend() = 0;
    virtual void EnableDepthTest() = 0;

    // NOTE(v.matushkin): Questionable methods
    // OpenGL GLuint
    // DX11   ID3D11ShaderResourceView*
    // DX12   D3D12_GPU_DESCRIPTOR_HANDLE
    virtual [[nodiscard]] void*            GetNativeRenderTexture(RenderTextureHandle renderTextureHandle) = 0;
    virtual [[nodiscard]] RenderPassHandle GetSwapchainRenderPass() = 0;

    virtual void SetBlendFunction(BlendMode source, BlendMode destination) = 0;
    virtual void SetClearColor(f32 r, f32 g, f32 b, f32 a) = 0;
    virtual void SetDepthFunction(DepthCompareFunction depthCompareFunction) = 0;
    virtual void SetViewport(i32 x, i32 y, i32 width, i32 height) = 0;

    virtual void Clear(BufferBit bufferBitMask) = 0;

    // TODO(v.matushkin): Remove, temporary method
    virtual void BeginFrame(const glm::mat4x4& localToWorld, const glm::mat4x4& cameraView, const glm::mat4x4& cameraProjection) = 0;
    virtual void BeginRenderPass(RenderPassHandle renderPassHandle) = 0;
    virtual void BeginRenderPass(RenderPassHandle renderPassHandle, RenderTextureHandle input) = 0;
    virtual void EndRenderPass() = 0;
    virtual void EndFrame() = 0;

    // NOTE(v.matushkin): Questionable method
    virtual void DrawBuffer(TextureHandle textureHandle, BufferHandle bufferHandle, i32 indexCount, i32 vertexCount) = 0;

    virtual [[nodiscard]] IImGuiRenderContext* CreateImGuiRenderContext() = 0;

    virtual [[nodiscard]] RenderTextureHandle CreateRenderTexture(const RenderTextureDesc& renderTextureDesc) = 0;
    virtual [[nodiscard]] RenderPassHandle    CreateRenderPass(const RenderPassDesc& renderPassDesc) = 0;

    virtual [[nodiscard]] BufferHandle  CreateBuffer(
        std::span<const std::byte>              indexData,
        std::span<const std::byte>              vertexData,
        const std::vector<VertexAttributeDesc>& vertexLayout
    ) = 0;
    virtual [[nodiscard]] TextureHandle CreateTexture(const TextureDesc& textureDesc, const ui8* textureData) = 0;
    virtual [[nodiscard]] ShaderHandle  CreateShader(const ShaderDesc& shaderDesc) = 0;
};

} // namespace snv
