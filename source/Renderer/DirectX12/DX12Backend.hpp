#pragma once

#include <Renderer/IRendererBackend.hpp>
#include <Renderer/DirectX12/DX12ShaderCompiler.hpp>

#include <d3d12.h>
#include <glm/ext/matrix_float4x4.hpp>
#include <wrl/client.h>

#include <unordered_map>


struct IDXGIFactory7;
struct IDXGISwapChain4;
// NOTE(v.matushkin): Can't forward this because of D3D12_VERTEX_BUFFER_VIEW/D3D12_INDEX_BUFFER_VIEW structs
//  May be just define my own, with the same layout?
// struct ID3D12Device8;
// struct ID3D12CommandQueue;
// struct ID3D12Resource2;
// struct ID3D12GraphicsCommandList6;
// struct ID3D12CommandAllocator;
// struct ID3D12DescriptorHeap;
// struct ID3D12Fence1;


namespace snv
{

class DX12Backend final : public IRendererBackend
{
    struct DX12Buffer
    {
        Microsoft::WRL::ComPtr<ID3D12Resource2> Index;
        Microsoft::WRL::ComPtr<ID3D12Resource2> Position;
        Microsoft::WRL::ComPtr<ID3D12Resource2> Normal;
        Microsoft::WRL::ComPtr<ID3D12Resource2> TexCoord0;

        D3D12_INDEX_BUFFER_VIEW  IndexView;
        D3D12_VERTEX_BUFFER_VIEW PositionView;
        D3D12_VERTEX_BUFFER_VIEW NormalView;
        D3D12_VERTEX_BUFFER_VIEW TexCoord0View;
    };

    struct DX12Texture
    {
        Microsoft::WRL::ComPtr<ID3D12Resource2> Texture;
        ui32                                    IndexInDescriptorHeap;
    };

    struct DX12Shader
    {
        DX12ShaderBytecode VertexShader;
        DX12ShaderBytecode FragmentShader;
    };


    // TODO(v.matushkin): PerFrame/PerDraw should be declared in some common header
    struct alignas(256) PerFrame
    {
        glm::mat4x4 _CameraView;
        glm::mat4x4 _CameraProjection;
    };
    struct alignas(256) PerDraw
    {
        glm::mat4x4 _ObjectToWorld;
    };


    static const ui32 k_BackBufferFrames = 3;

public:
    DX12Backend();
    ~DX12Backend();

    void EnableBlend() override;
    void EnableDepthTest() override;

    void SetBlendFunction(BlendFactor source, BlendFactor destination) override;
    void SetClearColor(f32 r, f32 g, f32 b, f32 a) override;
    void SetDepthFunction(DepthFunction depthFunction) override;
    void SetViewport(i32 x, i32 y, i32 width, i32 height) override;

    void Clear(BufferBit bufferBitMask) override;

    void BeginFrame(const glm::mat4x4& localToWorld, const glm::mat4x4& cameraView, const glm::mat4x4& cameraProjection) override;
    void EndFrame() override;
    void DrawBuffer(TextureHandle textureHandle, BufferHandle bufferHandle, i32 indexCount, i32 vertexCount) override;
    void DrawArrays(i32 count) override;
    void DrawElements(i32 count) override;

    BufferHandle CreateBuffer(
        std::span<const std::byte>              indexData,
        std::span<const std::byte>              vertexData,
        const std::vector<VertexAttributeDesc>& vertexLayout
    ) override;
    TextureHandle CreateTexture(const TextureDesc& textureDesc, const ui8* textureData) override;
    ShaderHandle  CreateShader(std::span<const char> vertexSource, std::span<const char> fragmentSource) override;

private:
    void CreateDevice();
    void CreateCommandQueue();
    void CreateSwapChain();
    void CreateDescriptorHeaps();
    void CreateRenderTargetViews();
    void CreateDepthStencilView();
    void CreateCommandAllocators();
    void CreateCommandList();
    void CreateFence();
    void CreateConstantBuffer(ID3D12Resource2** constantBuffer, ui32 size);

    void CreateRootSignature();
    void CreatePipeline();

    void WaitForPreviousFrame();

    bool CheckTearingSupport();

private:
    //-----------------------------------------------------------------------------
    // Direct3D resources
    //-----------------------------------------------------------------------------
    Microsoft::WRL::ComPtr<IDXGIFactory7>              m_factory;
    Microsoft::WRL::ComPtr<ID3D12Device8>              m_device;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue>         m_commandQueue;

    Microsoft::WRL::ComPtr<IDXGISwapChain4>            m_swapChain;
    Microsoft::WRL::ComPtr<ID3D12Resource2>            m_backBuffers[k_BackBufferFrames];

    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>     m_commandAllocators[k_BackBufferFrames];
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6> m_graphicsCommandList;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>       m_descriptorHeapRTV;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>       m_descriptorHeapDSV;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>       m_descriptorHeapSRV;

    Microsoft::WRL::ComPtr<ID3D12RootSignature>        m_rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>        m_graphicsPipeline;

    Microsoft::WRL::ComPtr<ID3D12Resource2> m_depthStencil;

    Microsoft::WRL::ComPtr<ID3D12Resource2> m_cbPerFrame;
    Microsoft::WRL::ComPtr<ID3D12Resource2> m_cbPerDraw;
    PerFrame                                m_cbPerFrameData;
    PerDraw                                 m_cbPerDrawData;

    D3D12_VIEWPORT m_viewport;
    D3D12_RECT     m_scissorRect;

    ui32 m_rtvDescriptorSize;
    ui32 m_srvDescriptorSize; // CBV/SRV/UAV
    ui32 m_currentBackBufferIndex;

    // Synchronization objects
    Microsoft::WRL::ComPtr<ID3D12Fence1> m_fence;
    ui64                                 m_fenceValue;
    ui64                                 m_frameFenceValues[k_BackBufferFrames];
    void*                                m_fenceEvent;


    DX12ShaderCompiler m_shaderCompiler;

    f32 m_clearColor[4] = {0.098f, 0.439f, 0.439f, 1.000f};

    std::unordered_map<BufferHandle, DX12Buffer>   m_buffers;
    std::unordered_map<TextureHandle, DX12Texture> m_textures;
    std::unordered_map<ShaderHandle, DX12Shader>   m_shaders;
};

} // namespace snv
