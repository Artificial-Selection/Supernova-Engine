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
        RenderTextureClearValue ClearValue;
        RenderTextureLoadAction LoadAction;
    };

    struct GLFramebuffer
    {
        ui32                         ID;
        std::vector<GLRenderTexture> ColorAttachments;
        GLRenderTexture              DepthStencilAttachment;
        ui32                         DepthStencilType;
    };

public:
    GLBackend();
    ~GLBackend() override = default; // TODO(v.matushkin): Destroy OpenGL resources

    void EnableBlend() override;
    void EnableDepthTest() override;

    [[nodiscard]] void*             GetNativeRenderTexture(RenderTextureHandle renderTextureHandle) override;
    [[nodiscard]] FramebufferHandle GetSwapchainFramebuffer() override { return m_swapchainFramebufferHandle; }

    void SetBlendFunction(BlendMode source, BlendMode destination) override;
    void SetClearColor(f32 r, f32 g, f32 b, f32 a) override;
    void SetDepthFunction(DepthCompareFunction depthCompareFunction) override;
    void SetViewport(i32 x, i32 y, i32 width, i32 height) override;

    void Clear(BufferBit bufferBitMask) override;

    void BeginFrame(const glm::mat4x4& localToWorld, const glm::mat4x4& cameraView, const glm::mat4x4& cameraProjection) override;
    void BeginRenderPass(FramebufferHandle framebufferHandle) override;
    void BeginRenderPass(FramebufferHandle framebufferHandle, RenderTextureHandle input) override;
    void EndFrame() override;

    void DrawBuffer(TextureHandle textureHandle, BufferHandle bufferHandle, i32 indexCount, i32 vertexCount) override;

    [[nodiscard]] IImGuiRenderContext* CreateImGuiRenderContext() override;

    [[nodiscard]] GraphicsState CreateGraphicsState(const GraphicsStateDesc& graphicsStateDesc) override;
    [[nodiscard]] BufferHandle  CreateBuffer(
        std::span<const std::byte>              indexData,
        std::span<const std::byte>              vertexData,
        const std::vector<VertexAttributeDesc>& vertexLayout
    ) override;
    [[nodiscard]] TextureHandle CreateTexture(const TextureDesc& textureDesc, const ui8* textureData) override;
    [[nodiscard]] ShaderHandle  CreateShader(const ShaderDesc& shaderDesc) override;

private:
    FramebufferHandle m_swapchainFramebufferHandle;

    std::unordered_map<BufferHandle,        GLBuffer>        m_buffers;
    std::unordered_map<FramebufferHandle,   GLFramebuffer>   m_framebuffers;
    std::unordered_map<RenderTextureHandle, GLRenderTexture> m_renderTextures;
    std::unordered_map<ShaderHandle,        GLShader>        m_shaders;
    std::unordered_map<TextureHandle,       GLTexture>       m_textures;
};

} // namespace snv
