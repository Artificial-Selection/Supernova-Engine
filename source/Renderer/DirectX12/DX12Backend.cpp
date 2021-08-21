#include <Renderer/DirectX12/DX12Backend.hpp>

#include <Core/Assert.hpp>
#include <Core/Window.hpp>

#include <d3d12.h>
#include <dxgi1_6.h>

#include <exception>
#include <string>


#define THROW_IF_FAILED(hresult)    \
    {                               \
        if (FAILED(hresult))        \
        {                           \
            throw std::exception(); \
        }                           \
    }                               \

// void D3D12MessageCallback(
//     D3D12_MESSAGE_CATEGORY category, D3D12_MESSAGE_SEVERITY severity, D3D12_MESSAGE_ID id, const char* pDescription, void* pContext
// )
// {
//     switch (severity)
//     {
//         // TODO(v.matushkin): Should be LOG_TRACE, but it doesn't work rn
//         case D3D12_MESSAGE_SEVERITY::D3D12_MESSAGE_SEVERITY_MESSAGE:
//             LOG_INFO(
//                 "DirectX12 debug message\n\t{0}\n\tID: {1}\n\tCategory: {2}\n\tSeverity: MESSAGE",
//                 pDescription, id, category
//             );
//             break;
//         case D3D12_MESSAGE_SEVERITY::D3D12_MESSAGE_SEVERITY_INFO:
//             LOG_INFO(
//                 "DirectX12 debug message\n\t{0}\n\tID: {1}\n\tCategory: {2}\n\tSeverity: INFO",
//                 pDescription, id, category
//             );
//             break;
//         case D3D12_MESSAGE_SEVERITY::D3D12_MESSAGE_SEVERITY_WARNING:
//             LOG_WARN(
//                 "DirectX12 debug message\n\t{0}\n\tID: {1}\n\tCategory: {2}\n\tSeverity: WARNING",
//                 pDescription, id, category
//             );
//             break;
//         case D3D12_MESSAGE_SEVERITY::D3D12_MESSAGE_SEVERITY_ERROR:
//             LOG_ERROR(
//                 "DirectX12 debug message\n\t{0}\n\tID: {1}\n\tCategory: {2}\n\tSeverity: ERROR",
//                 pDescription, id, category
//             );
//             break;
//         case D3D12_MESSAGE_SEVERITY::D3D12_MESSAGE_SEVERITY_CORRUPTION:
//             LOG_CRITICAL(
//                 "DirectX12 debug message\n\t{0}\n\tID: {1}\n\tCategory: {2}\n\tSeverity: CORRUPTION",
//                 pDescription, id, category
//             );
//             break;
//     }
// }


namespace snv
{

DX12Backend::DX12Backend()
    : m_currentBackBufferIndex(0)
{
    CreateDevice();
    CreateCommandQueue();
    CreateSwapChain();
    CreateDescriptorHeap();
    CreateRenderTargetViews();
    CreateCommandAllocators();
    CreateCommandList();
    CreateFence();
}


void DX12Backend::EnableBlend()
{}

void DX12Backend::EnableDepthTest()
{}


void DX12Backend::SetBlendFunction(BlendFactor source, BlendFactor destination)
{}

void DX12Backend::SetClearColor(f32 r, f32 g, f32 b, f32 a)
{
    m_clearColor[0] = r;
    m_clearColor[1] = g;
    m_clearColor[2] = b;
    m_clearColor[3] = a;
}

void DX12Backend::SetDepthFunction(DepthFunction depthFunction)
{}

void DX12Backend::SetViewport(i32 x, i32 y, i32 width, i32 height)
{}


void DX12Backend::Clear(BufferBit bufferBitMask)
{}


void DX12Backend::BeginFrame(const glm::mat4x4& localToWorld, const glm::mat4x4& cameraView, const glm::mat4x4& cameraProjection)
{
    // Command list allocators can only be reset when the associated command lists have finished execution on the GPU
    auto commandAllocator = m_commandAllocators[m_currentBackBufferIndex].Get();

    commandAllocator->Reset();
    m_graphicsCommandList->Reset(commandAllocator, nullptr);

    // Transition render target from PRESENT to RENDER_TARGET
    D3D12_RESOURCE_TRANSITION_BARRIER d3dResourceTransitionBarrier = {
        .pResource   = m_backBuffers[m_currentBackBufferIndex].Get(),
        .Subresource = 0, // ???
        .StateBefore = D3D12_RESOURCE_STATE_PRESENT,
        .StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET
    };
    D3D12_RESOURCE_BARRIER d3dResourceBarrier = {
        .Type       = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        .Flags      = D3D12_RESOURCE_BARRIER_FLAG_NONE, // ???
        .Transition = d3dResourceTransitionBarrier
    };
    m_graphicsCommandList->ResourceBarrier(1, &d3dResourceBarrier);

    // Clear render target
    auto rtvDescriptorHandle = m_descriptorHeapRTV->GetCPUDescriptorHandleForHeapStart();
    rtvDescriptorHandle.ptr += m_rtvDescriptorSize * m_currentBackBufferIndex;
    m_graphicsCommandList->ClearRenderTargetView(rtvDescriptorHandle, m_clearColor, 0, nullptr);
}

void DX12Backend::EndFrame()
{
    // Transition render target from RENDER_TARGET to PRESENT
    D3D12_RESOURCE_TRANSITION_BARRIER d3dResourceTransitionBarrier = {
        .pResource   = m_backBuffers[m_currentBackBufferIndex].Get(),
        .Subresource = 0, // ???
        .StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET,
        .StateAfter  = D3D12_RESOURCE_STATE_PRESENT
    };
    D3D12_RESOURCE_BARRIER d3dResourceBarrier = {
        .Type       = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        .Flags      = D3D12_RESOURCE_BARRIER_FLAG_NONE, // ???
        .Transition = d3dResourceTransitionBarrier
    };
    m_graphicsCommandList->ResourceBarrier(1, &d3dResourceBarrier);

    m_graphicsCommandList->Close();

    // Execute command list
    ID3D12CommandList* commandLists[] = {m_graphicsCommandList.Get()};
    m_commandQueue->ExecuteCommandLists(1, commandLists);

    // Present frame
    // TODO(v.matushkin): Present1() ?
    // NOTE(v.matushkin): VSync? Tearing?
    m_swapChain->Present(1, 0);

    WaitForPreviousFrame();
}

void DX12Backend::DrawBuffer(TextureHandle textureHandle, BufferHandle bufferHandle, i32 indexCount, i32 vertexCount)
{}

void DX12Backend::DrawArrays(i32 count)
{}

void DX12Backend::DrawElements(i32 count)
{}


BufferHandle DX12Backend::CreateBuffer(
    std::span<const std::byte>              indexData,
    std::span<const std::byte>              vertexData,
    const std::vector<VertexAttributeDesc>& vertexLayout
)
{
    return BufferHandle();
}

TextureHandle DX12Backend::CreateTexture(const TextureDesc& textureDesc, const ui8* textureData)
{
    return TextureHandle();
}

ShaderHandle DX12Backend::CreateShader(std::span<const char> vertexSource, std::span<const char> fragmentSource)
{
    return ShaderHandle();
}


void DX12Backend::CreateDevice()
{
    ui32 dxgiFactoryFlags = 0;

#ifdef SNV_GPU_API_DEBUG_ENABLED
    Microsoft::WRL::ComPtr<ID3D12Debug3> d3dDebug;
    D3D12GetDebugInterface(IID_PPV_ARGS(d3dDebug.GetAddressOf()));
    d3dDebug->EnableDebugLayer();
    // hr = d3dDebug.As(&d3dDebug5);
    // NOTE(v.matushkin): Use SetEnableGPUBasedValidation() and
    //  SetEnableAutoDebugName() from ID3D12Debug5?
    //  Although for some fucking reason, ID3D12Debug3 is the max version I can get
    //  And apparently I should set resource names to ease the debug

    dxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif // SNV_GPU_API_DEBUG_ENABLED

    CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(m_factory.GetAddressOf()));

    Microsoft::WRL::ComPtr<IDXGIAdapter4> dxgiAdapter;
    const auto dxgiGPUPreference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
    const auto riid = __uuidof(IDXGIAdapter4);
    auto ppvAdapter = IID_PPV_ARGS_Helper(dxgiAdapter.GetAddressOf());

    const auto hr = m_factory->EnumAdapterByGpuPreference(0, dxgiGPUPreference, riid, ppvAdapter);
    SNV_ASSERT(hr != DXGI_ERROR_NOT_FOUND, "Failed to find DX12 device");
    {
        DXGI_ADAPTER_DESC3 dxgiAdapterDesc;
        dxgiAdapter->GetDesc3(&dxgiAdapterDesc);
        // TODO: Fix this shit with spdlog wide string option support? (need custom conan package then)
        std::wstring wstr(dxgiAdapterDesc.Description);
        std::string  desc(wstr.begin(), wstr.end());
        LOG_INFO("DirectX 12\nAdapter description: {}", desc);
    }

    // TODO(v.matushkin): Wait until I can get D3D_FEATURE_LEVEL_12_2 for ID3D12Device9/ID3D12InfoQueue1
    D3D12CreateDevice(dxgiAdapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(m_device.GetAddressOf()));

#ifdef SNV_GPU_API_DEBUG_ENABLED
    // Microsoft::WRL::ComPtr<ID3D12InfoQueue1> d3dInfoQueue;
    // hr = m_device.As(&d3dInfoQueue);

    // DWORD useless_callback_cookie; // NOTE(v.matushkin): Used only for UnregisterMessageCallback() ?
    // d3dInfoQueue->RegisterMessageCallback(D3D12MessageCallback, D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr, &useless_callback_cookie);
    // NOTE(v.matushkin): Use SetBreakOnSeverity() and PushStorageFilter() ?
#endif // SNV_GPU_API_DEBUG_ENABLED
}

void DX12Backend::CreateCommandQueue()
{
    // NOTE(v.matushkin): One day I wll try multi-gpu
    D3D12_COMMAND_QUEUE_DESC d3dCommandQueueDesc = {
        .Type     = D3D12_COMMAND_LIST_TYPE_DIRECT,
        .Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH,
        .Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE,
        .NodeMask = 0   // NOTE(v.matushkin): From docs "For single GPU operation, set this to zero."
    };

    m_device->CreateCommandQueue(&d3dCommandQueueDesc, IID_PPV_ARGS(m_commandQueue.GetAddressOf()));
}

void DX12Backend::CreateSwapChain()
{
    // NOTE(v.matushkin): This member is valid only with bit-block transfer (bitblt) model swap chains.
    //  When using flip model swap chain, this member must be specified as {1, 0}
    DXGI_SAMPLE_DESC dxgiSwapChainSampleDesc = {
        .Count   = 1, // multisampling setting
        .Quality = 0  // vendor-specific flag
    };
    ui32 allowTearingFlag = CheckTearingSupport() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
    ui32 dxgiSwapChainFlags = allowTearingFlag;
                            // | DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
    // NOTE(v.matushkin): From bgfx:
    //  According to https://docs.microsoft.com/en-us/windows/win32/direct3ddxgi/converting-data-color-space ,
    //  it is OK to create the backbuffer with m_fmt (a non- _SRGB format), and then create the render target view into it
    //  with m_fmtSrgb (the _SRGB format of same), and it will work identically as if you had created the swapchain with
    //  m_fmtSrgb as the backbuffer format.
    //
    //  Moreover, it is actually not desirable to create the backbuffer with an _SRGB format, because that
    //  is incompatible with the flip presentation model, which is desirable for various reasons including
    //  player embedding.
    DXGI_SWAP_CHAIN_DESC1 dxgiSwapChainDesc = {
        // .Width,                                      // NOTE(v.matushkin): Specify Width/Height explicitly?
        // .Height
        .Format      = DXGI_FORMAT_R8G8B8A8_UNORM,      // TODO(v.matushkin): Shouldn't be hardcoded
        .Stereo      = false,
        .SampleDesc  = dxgiSwapChainSampleDesc,
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = k_BackBufferFrames,
        .Scaling     = DXGI_SCALING_STRETCH,            // TODO(v.matushkin): Play with this
        .SwapEffect  = DXGI_SWAP_EFFECT_FLIP_DISCARD,
        .AlphaMode   = DXGI_ALPHA_MODE_UNSPECIFIED,     // NOTE(v.matushkin): Don't know
        .Flags       = dxgiSwapChainFlags
    };

    const auto win32Window = Window::GetWin32Window();
    // NOTE(v.matushkin): Why? And why this called in CreateSwapChain() method? Shouldn't this be configurable?
    // Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen will be handled manually.
    m_factory->MakeWindowAssociation(win32Window, DXGI_MWA_NO_ALT_ENTER);

    Microsoft::WRL::ComPtr<IDXGISwapChain1> dxgiSwapChain;
    m_factory->CreateSwapChainForHwnd(
        m_commandQueue.Get(), // Swap chain needs the queue so that it can force a flush on it.
        win32Window,
        &dxgiSwapChainDesc,
        nullptr,
        nullptr,
        dxgiSwapChain.GetAddressOf()
    );
    dxgiSwapChain.As(&m_swapChain);
}

void DX12Backend::CreateDescriptorHeap()
{
    D3D12_DESCRIPTOR_HEAP_DESC d3dDescriptorHeapDesc = {
        .Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        .NumDescriptors = k_BackBufferFrames,
        .Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE, // This flag only applies to CBV, SRV, UAV and samplers
        .NodeMask       = 0
    };
     m_device->CreateDescriptorHeap(&d3dDescriptorHeapDesc, IID_PPV_ARGS(m_descriptorHeapRTV.GetAddressOf()));
}

void DX12Backend::CreateRenderTargetViews()
{
    m_rtvDescriptorSize      = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    auto rtvDescriptorHandle = m_descriptorHeapRTV->GetCPUDescriptorHandleForHeapStart();

    for (ui32 i = 0; i < k_BackBufferFrames; ++i)
    {
        auto& backBuffer = m_backBuffers[i];

        m_swapChain->GetBuffer(i, IID_PPV_ARGS(backBuffer.GetAddressOf()));
        // NOTE(v.matushkin): D3D12_RENDER_TARGET_VIEW_DESC = null?
        m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvDescriptorHandle);

        rtvDescriptorHandle.ptr += m_rtvDescriptorSize;
    }
}

void DX12Backend::CreateCommandAllocators()
{
    for (ui32 i = 0; i < k_BackBufferFrames; ++i)
    {
        m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_commandAllocators[i].GetAddressOf()));
    }
}

void DX12Backend::CreateCommandList()
{
    // NOTE(v.matushkin): Nice empty D3D12_COMMAND_LIST_FLAGS enum
    m_device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(m_graphicsCommandList.GetAddressOf()));
    // NOTE(v.matushkin): Apparently there is no need to call Close() if CommandList was created with CreateCommandList1()
    //  Nice docs microsoft, zero fucking info on what is the diff between CreateCommandList and CreateCommandList1
    // Command lists are created in the recording state, but there is nothing to record yet.
    // m_graphicsCommandList->Close();
}

void DX12Backend::CreateFence()
{
    m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fence.GetAddressOf()));

    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    m_fenceValue = 1;
}


void DX12Backend::WaitForPreviousFrame()
{
    m_commandQueue->Signal(m_fence.Get(), m_fenceValue);

    if (m_fence->GetCompletedValue() < m_fenceValue)
    {
        m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent);
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }

    m_fenceValue++;
    m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
}


bool DX12Backend::CheckTearingSupport()
{
    i32 allowTearing = 0;
    m_factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));

    return allowTearing == 1;
}

} // namespace snv
