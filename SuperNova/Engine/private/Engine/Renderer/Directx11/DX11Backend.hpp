#pragma once

#include <Engine/Renderer/IRendererBackend.hpp>

#include <glm/ext/matrix_float4x4.hpp>
#include <wrl/client.h>

#include <unordered_map>


struct ID3D11Device5;
struct ID3D11DeviceContext4;
struct IDXGISwapChain4;
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
    struct DX11Buffer
    {
        Microsoft::WRL::ComPtr<ID3D11Buffer> Index;
        Microsoft::WRL::ComPtr<ID3D11Buffer> Position;
        Microsoft::WRL::ComPtr<ID3D11Buffer> Normal;
        Microsoft::WRL::ComPtr<ID3D11Buffer> TexCoord0;
    };

    struct DX11Texture
    {
        Microsoft::WRL::ComPtr<ID3D11Texture2D1>         Texture;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV;
        Microsoft::WRL::ComPtr<ID3D11SamplerState>       Sampler;
    };

    struct DX11Shader
    {
        Microsoft::WRL::ComPtr<ID3D11InputLayout>  InputLayout;
        Microsoft::WRL::ComPtr<ID3D11VertexShader> VertexShader;
        Microsoft::WRL::ComPtr<ID3D11PixelShader>  FragmentShader;
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

    void EnableBlend() override;
    void EnableDepthTest() override;

    [[nodiscard]] void* GetNativeRenderTexture(RenderTextureHandle renderTextureHandle) override { return nullptr; }

    void SetBlendFunction(BlendFactor source, BlendFactor destination) override;
    void SetClearColor(f32 r, f32 g, f32 b, f32 a) override;
    void SetDepthFunction(DepthFunction depthFunction) override;
    void SetViewport(i32 x, i32 y, i32 width, i32 height) override;

    void Clear(BufferBit bufferBitMask) override;

    void BeginFrame(const glm::mat4x4& localToWorld, const glm::mat4x4& cameraView, const glm::mat4x4& cameraProjection) override;
    void BeginRenderPass(FramebufferHandle framebufferHandle) override {}
    void EndFrame() override;

    void DrawBuffer(TextureHandle textureHandle, BufferHandle bufferHandle, i32 indexCount, i32 vertexCount) override;

    [[nodiscard]] GraphicsState CreateGraphicsState(const GraphicsStateDesc& graphicsStateDesc) override { return {}; }
    [[nodiscard]] BufferHandle  CreateBuffer(
        std::span<const std::byte>              indexData,
        std::span<const std::byte>              vertexData,
        const std::vector<VertexAttributeDesc>& vertexLayout
    ) override;
    [[nodiscard]] TextureHandle CreateTexture(const TextureDesc& textureDesc, const ui8* textureData) override;
    [[nodiscard]] ShaderHandle  CreateShader(std::span<const char> vertexSource, std::span<const char> fragmentSource) override;

private:
    void CreateDevice();
    void CreateSwapChain();

private:
    //-----------------------------------------------------------------------------
    // Direct3D resources
    //-----------------------------------------------------------------------------
    Microsoft::WRL::ComPtr<ID3D11Device5>        m_device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext4> m_deviceContext;
    Microsoft::WRL::ComPtr<IDXGISwapChain4>      m_swapChain;
    // Back buffer
    Microsoft::WRL::ComPtr<ID3D11Texture2D1>       m_renderTarget;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_renderTargetView;
    Microsoft::WRL::ComPtr<ID3D11Texture2D1>       m_depthStencil;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_depthStencilView;
    // Constant buffers
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_cbPerFrame;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_cbPerDraw;


    f32 m_clearColor[4] = {0.098f, 0.439f, 0.439f, 1.000f};

    PerFrame m_cbPerFrameData;
    PerDraw  m_cbPerDrawData;

    std::unordered_map<BufferHandle,  DX11Buffer>  m_buffers;
    std::unordered_map<TextureHandle, DX11Texture> m_textures;
    std::unordered_map<ShaderHandle,  DX11Shader>  m_shaders;
};

} // namespace snv
