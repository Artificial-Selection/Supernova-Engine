#pragma once

#include <Renderer/IRendererBackend.hpp>

#include <glm/ext/matrix_float4x4.hpp>
#include <wrl/client.h>


struct IDXGIFactory7;
struct ID3D12Device8;
struct ID3D12CommandQueue;
struct IDXGISwapChain4;
struct ID3D12Resource2;
struct ID3D12GraphicsCommandList6;
struct ID3D12CommandAllocator;
struct ID3D12DescriptorHeap;
struct ID3D12Fence1;


namespace snv
{

class DX12Backend final : public IRendererBackend
{
    static const ui32 k_BackBufferFrames = 3;

public:
    DX12Backend();

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
    void CreateDescriptorHeap();
    void CreateRenderTargetViews();
    void CreateCommandAllocators();
    void CreateCommandList();
    void CreateFence();

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

    ui32 m_rtvDescriptorSize;
    ui32 m_currentBackBufferIndex;

    // Synchronization resources
    Microsoft::WRL::ComPtr<ID3D12Fence1> m_fence;
    ui64                                 m_fenceValue;
    ui64                                 m_frameFenceValues[k_BackBufferFrames];
    void*                                m_fenceEvent;


    f32 m_clearColor[4] = {0.098f, 0.439f, 0.439f, 1.000f};
};

} // namespace snv
