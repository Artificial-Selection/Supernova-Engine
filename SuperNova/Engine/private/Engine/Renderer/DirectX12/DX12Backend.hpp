#pragma once

#include <Engine/Renderer/IRendererBackend.hpp>

#include <d3d12.h>
#include <glm/ext/matrix_float4x4.hpp>
#include <wrl/client.h>

#include <memory>
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

class DX12DescriptorHeap;


class DX12Backend final : public IRendererBackend
{
    template<class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;


    struct DX12Buffer
    {
        ComPtr<ID3D12Resource2> Index;
        ComPtr<ID3D12Resource2> Position;
        ComPtr<ID3D12Resource2> Normal;
        ComPtr<ID3D12Resource2> TexCoord0;

        D3D12_INDEX_BUFFER_VIEW  IndexView;
        D3D12_VERTEX_BUFFER_VIEW PositionView;
        D3D12_VERTEX_BUFFER_VIEW NormalView;
        D3D12_VERTEX_BUFFER_VIEW TexCoord0View;
    };

    struct DX12RenderTexture
    {
        ComPtr<ID3D12Resource2>     Texture;
        D3D12_CPU_DESCRIPTOR_HANDLE Descriptor;
        D3D12_CPU_DESCRIPTOR_HANDLE SrvCpuDescriptor;
        D3D12_GPU_DESCRIPTOR_HANDLE SrvGpuDescriptor;
        D3D12_RESOURCE_STATES       CurrentState;
        RenderTextureClearValue     ClearValue;
        RenderTextureLoadAction     LoadAction;
        RenderTextureType           Type; // NOTE(v.matushkin): May be store DepthStencilClearFlags instead of type?
    };
    using DX12RenderTexturePtr = std::shared_ptr<DX12RenderTexture>;

    struct DX12RenderPass
    {
        std::vector<DX12RenderTexturePtr>        ColorAttachments;
        std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> ColorDescriptors;
        DX12RenderTexturePtr                     DepthStencilAttachment;
        D3D12_CLEAR_FLAGS                        DepthStencilClearFlags;
    };

    struct DX12Shader
    {
        // NOTE(v.matushkin): I'm sure compiled shader blobs will be needed, but not know
        // DX12ShaderBytecode VertexShader;
        // DX12ShaderBytecode FragmentShader;
        ComPtr<ID3D12PipelineState> GraphicsPipeline;
    };

    struct DX12Texture
    {
        ComPtr<ID3D12Resource2>     Texture;
        D3D12_CPU_DESCRIPTOR_HANDLE CpuDescriptor;
        D3D12_GPU_DESCRIPTOR_HANDLE GpuDescriptor;
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
    void EndRenderPass() override {}
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
    void CreateDevice();
    void CreateCommandQueue();
    void CreateSwapchain();
    void CreateCommandAllocators();
    void CreateCommandList();
    void CreateFence();
    void CreateConstantBuffer(ID3D12Resource2** constantBuffer, ui32 size);

    void CreateRootSignature();

    void WaitForPreviousFrame();

    bool CheckTearingSupport();

private:
    //-----------------------------------------------------------------------------
    // Direct3D resources
    //-----------------------------------------------------------------------------
    ComPtr<IDXGIFactory7>              m_factory;
    ComPtr<ID3D12Device8>              m_device;
    ComPtr<ID3D12CommandQueue>         m_commandQueue;

    ComPtr<IDXGISwapChain4>            m_swapChain;

    ComPtr<ID3D12CommandAllocator>     m_commandAllocators[k_BackBufferFrames];
    ComPtr<ID3D12GraphicsCommandList6> m_graphicsCommandList;

    ComPtr<ID3D12RootSignature>        m_rootSignature;

    DX12RenderPass                     m_swapchainRenderPasses[k_BackBufferFrames];
    RenderPassHandle                   m_swapchainRenderPassHandle;
    ui32                               m_currentBackBufferIndex;

    ComPtr<ID3D12Resource2>            m_cbPerFrame;
    ComPtr<ID3D12Resource2>            m_cbPerDraw;
    PerFrame                           m_cbPerFrameData;
    PerDraw                            m_cbPerDrawData;

    D3D12_VIEWPORT                     m_viewport;
    D3D12_RECT                         m_scissorRect;

    // Synchronization objects
    ComPtr<ID3D12Fence1>               m_fence;
    ui64                               m_fenceValue;
    ui64                               m_frameFenceValues[k_BackBufferFrames];
    void*                              m_fenceEvent;


    std::unique_ptr<DX12DescriptorHeap> m_descriptorHeap;

    // f32 m_clearColor[4] = {0.098f, 0.439f, 0.439f, 1.000f}; // TODO(v.matushkin): Remove? Not used right now

    std::unordered_map<BufferHandle,        DX12Buffer>           m_buffers;
    std::unordered_map<RenderPassHandle,    DX12RenderPass>       m_renderPasses;
    std::unordered_map<RenderTextureHandle, DX12RenderTexturePtr> m_renderTextures;
    std::unordered_map<ShaderHandle,        DX12Shader>           m_shaders;
    std::unordered_map<TextureHandle,       DX12Texture>          m_textures;
};

} // namespace snv
