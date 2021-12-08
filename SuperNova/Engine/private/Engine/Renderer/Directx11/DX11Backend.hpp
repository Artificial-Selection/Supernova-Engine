#pragma once

#include <Engine/Renderer/IRendererBackend.hpp>

#include <glm/ext/matrix_float4x4.hpp>
#include <wrl/client.h>

#include <span>
#include <unordered_map>
#include <vector>


struct IDXGIFactory5;
struct IDXGISwapChain4;
struct ID3D11Device5;
struct ID3D11DeviceContext4;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct ID3D11InputLayout;
struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11Buffer;
struct ID3D11SamplerState;
struct ID3D11ShaderResourceView;
struct ID3D11Texture2D1;


namespace snv
{

class DX11Backend final : public IRendererBackend
{
    template<class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;


    struct DX11Buffer
    {
        ComPtr<ID3D11Buffer> Index;
        ComPtr<ID3D11Buffer> Position;
        ComPtr<ID3D11Buffer> Normal;
        ComPtr<ID3D11Buffer> TexCoord0;
    };

    struct DX11RenderTexture
    {
        ComPtr<ID3D11Texture2D1>         Texture;
        // NOTE(v.matushkin): RTV and DSV should be in union or std::variant, but it turns into a fuckin mess
        //  so fuck it, it's easier to leave it like this
        ComPtr<ID3D11RenderTargetView>   RTV;
        ComPtr<ID3D11DepthStencilView>   DSV;
        ComPtr<ID3D11ShaderResourceView> SRV; // NOTE(v.matushkin): Can be not set
        RenderTextureClearValue          ClearValue;
        RenderTextureLoadAction          LoadAction;
    };

    struct DX11Framebuffer
    {
        std::vector<DX11RenderTexture>       ColorAttachments;
        std::vector<ID3D11RenderTargetView*> ColorRTVs;
        DX11RenderTexture                    DepthStencilAttachment;
        ui32                                 DepthStencilClearFlags;
    };

    struct DX11Shader
    {
        ComPtr<ID3D11InputLayout>  InputLayout;
        ComPtr<ID3D11VertexShader> VertexShader;
        ComPtr<ID3D11PixelShader>  FragmentShader;
    };

    struct DX11Texture
    {
        ComPtr<ID3D11Texture2D1>         Texture;
        ComPtr<ID3D11ShaderResourceView> SRV;
        ComPtr<ID3D11SamplerState>       Sampler;
    };

    // TODO(v.matushkin): PerFrame/PerDraw should be declared in some common header
    struct PerFrame
    {
        glm::mat4x4 _CameraView;
        glm::mat4x4 _CameraProjection;
    };

    struct PerDraw
    {
        glm::mat4x4 _ObjectToWorld;
    };

public:
    DX11Backend();
    ~DX11Backend() = default;

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
    void CreateDevice();
    void CreateSwapchain();

private:
    //-----------------------------------------------------------------------------
    // Direct3D resources
    //-----------------------------------------------------------------------------
    ComPtr<IDXGIFactory5>        m_factory;
    ComPtr<ID3D11Device5>        m_device;
    ComPtr<ID3D11DeviceContext4> m_deviceContext;
    ComPtr<IDXGISwapChain4>      m_swapChain;

    FramebufferHandle            m_swapchainFramebufferHandle;

    ComPtr<ID3D11Buffer>         m_cbPerFrame;
    ComPtr<ID3D11Buffer>         m_cbPerDraw;


    f32 m_clearColor[4] = {0.098f, 0.439f, 0.439f, 1.000f}; // TODO(v.matushkin): Remove? Not used right now

    PerFrame m_cbPerFrameData;
    PerDraw  m_cbPerDrawData;

    std::unordered_map<BufferHandle,        DX11Buffer>        m_buffers;
    std::unordered_map<FramebufferHandle,   DX11Framebuffer>   m_framebuffers;
    std::unordered_map<RenderTextureHandle, DX11RenderTexture> m_renderTextures;
    std::unordered_map<ShaderHandle,        DX11Shader>        m_shaders;
    std::unordered_map<TextureHandle,       DX11Texture>       m_textures;
};

} // namespace snv
