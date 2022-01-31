#pragma once

#include <Engine/Core/Core.hpp>
#include <Engine/Renderer/IRendererBackend.hpp>
#include <Engine/Renderer/OpenGL/GLBuffer.hpp>
#include <Engine/Renderer/OpenGL/GLShader.hpp>
#include <Engine/Renderer/OpenGL/GLTexture.hpp>

#include <span>
#include <unordered_map>
#include <vector>


namespace snv
{

class GLBackend final : public IRendererBackend
{
    // NOTE(v.matushkin): One GLRenderTexture is stored in m_renderTextures and GLFramebuffer, which can lead to some shit?
    struct GLRenderTexture
    {
        ui32                    ID;
        ui32                    DepthStencilAttachmentType; // NOTE(v.matushkin): Do I need to store it here and in GLRenderPass?
        RenderTextureClearValue ClearValue;
        bool                    IsRenderbuffer;
    };

    struct GLSubpass
    {
        ui32                         FramebufferID;
        ui32                         DepthStencilType;
        std::vector<ui8>             ClearColorIndices;
        std::vector<ClearColorValue> ClearColorValues;
        ClearDepthStencilValue       ClearDepthStencilValue;
        // Will only be set when DepthStencilType != OPENGL_NO_DEPTH_STENCIL
        bool                         ShouldClearDepthStencil;
    };

    struct GLRenderPass
    {
        GLSubpass Subpass;
    };

public:
    GLBackend();
    ~GLBackend() override = default; // TODO(v.matushkin): Destroy OpenGL resources

    [[nodiscard]] void*            GetNativeRenderTexture(RenderTextureHandle renderTextureHandle) override;
    [[nodiscard]] RenderPassHandle GetSwapchainRenderPass() override { return m_swapchainRenderPassHandle; }

    void EnableBlend() override {}
    void EnableDepthTest() override {}
    void SetBlendFunction(BlendMode source, BlendMode destination) override {}
    void SetClearColor(f32 r, f32 g, f32 b, f32 a) override {}
    void SetDepthFunction(CompareFunction depthCompareFunction) override {}
    void SetViewport(i32 x, i32 y, i32 width, i32 height) override {}
    void Clear(BufferBit bufferBitMask) override {}

    void BeginFrame(const glm::mat4x4& localToWorld, const glm::mat4x4& cameraView, const glm::mat4x4& cameraProjection) override;
    void BeginRenderPass(RenderPassHandle renderPassHandle) override;
    void BeginRenderPass(RenderPassHandle renderPassHandle, RenderTextureHandle input) override;
    void EndRenderPass() override;
    void EndFrame() override;

    void BindShader(ShaderHandle shaderHandle) override;

    void DrawBuffer(TextureHandle textureHandle, BufferHandle bufferHandle, i32 indexCount, i32 vertexCount) override;

    [[nodiscard]] IImGuiRenderContext* CreateImGuiRenderContext() override;

    [[nodiscard]] RenderTextureHandle CreateRenderTexture(const RenderTextureDesc& renderTextureDesc) override;
    [[nodiscard]] RenderPassHandle    CreateRenderPass(const RenderPassDesc& renderPassDesc) override;

    [[nodiscard]] BufferHandle  CreateBuffer(
        std::span<const std::byte>              indexData,
        std::span<const std::byte>              vertexData,
        const std::vector<VertexAttributeDesc>& vertexLayout
    ) override;
    [[nodiscard]] TextureHandle CreateTexture(const TextureDesc& textureDesc, const ui8* textureData) override;
    [[nodiscard]] ShaderHandle  CreateShader(const ShaderDesc& shaderDesc) override;

private:
    RenderPassHandle m_swapchainRenderPassHandle;
    ShaderHandle     m_engineShaderHandle;

    std::unordered_map<BufferHandle,        GLBuffer>        m_buffers;
    std::unordered_map<RenderPassHandle,    GLRenderPass>    m_renderPasses;
    std::unordered_map<RenderTextureHandle, GLRenderTexture> m_renderTextures;
    std::unordered_map<ShaderHandle,        GLShader>        m_shaders;
    std::unordered_map<TextureHandle,       GLTexture>       m_textures;
};

} // namespace snv
