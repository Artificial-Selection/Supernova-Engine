#include <Engine/Renderer/DirectX12/DX12Backend.hpp>
#include <Engine/Renderer/DirectX12/DX12DescriptorHeap.hpp>
#include <Engine/Renderer/DirectX12/DX12ImGuiRenderContext.hpp>

#include <Engine/EngineSettings.hpp>
#include <Engine/Application/Window.hpp>
#include <Engine/Core/Assert.hpp>

#include <dxgi1_6.h>

#include <string>


// TODO(v.matushkin):
//  - <ResourceDesc>
//    Change Create*Resource* methods:
//      - Pass textureData in TextureDesc
//      - Pass indexData, vertexData, vertexLayout in BufferDesc
//      - Pass vertexSource, fragmentSource in ShaderDesc
//
//  - <VSync>
//    Right now DX12Backend doesn't sync frame presentation and GL/DX11 do. Add a way to turn it on/off.
//    This is a high priority because right now there is no way to compare FPS between backends.
//    - <Tearing>
//      There is a method to check Tearing support but it's not used. I don't even know what this is and how it is connected to VSync (if at all?)
//
//  - <WaitForPreviousFrame>
//    I'm think WaitForPreviousFrame() doesn't work as it should.
//    It doesn't use triple buffering and just always waits for the previous frame to finish?
//
//  - <WindowResize>
//    Add support for window resizing (for every Backend), shouldn't be that hard.
//
//  - <DynamicTextureDescriptorHeap>
//    Right now the size of a m_descriptorHeapSRV is fixed and limited by k_MaxTextureDescriptors (quick workaround to get rendering of textures done)
//    But it should be dynamically resizable, I think it's not that hard to implement
//
//  - <ComPtr>
//    Replace Microsoft::WRL::ComPtr with my own. Don't know if it's worth it.
//
//  - <MeshCreation>
//    - <Rename>
//      Rename CreateBuffer -> CreateMesh (and DrawBuffer -> DrawMesh) in every Backend
//    - <Upload>
//      Right now meshes are just placed in UPLOAD heap, copy them to the GPU (same as textures)
//
//  - <CopyQueue>
//    Right now resources are copied on gpu through DIRECT queue. I should use COPY queue, especially for async resource upload(I guess?)
//    Also look into transition barriers, I saw somewhere that sometimes there are redundant,
//    and after some operations, resource is already in the right state. Does COPY queue helps with this somehow? (doubt that)
//
//  - <TextureCreation>
//    - <Upload>
//      Right now I just memcpy texture data to UPLOAD(CPU) heap. Things will get more complicated with mipmaps, texture arrays.
//      [LINKS]:
//        - http://alextardif.com/D3D11To12P3.html
//        - https://mynameismjp.wordpress.com/2016/03/25/bindless-texturing-for-deferred-rendering-and-decals/
//
//  - <TextureSampler>
//    Right now CreateTexture ignores texture wrapping options, there is only one static sampler. I don't know how to manage differnt samplers
//
//  - <AsynResourceUpload>
//    Right now CreateTexture(and later CreateMesh) call is blocking until it's done uploading to the GPU.
//    There should be async resource uploading API, although right now I don't know how I'm gonna synchronize it with the rendering
//      - Sync with graphcis command list:
//        IDK???
//      - Do not sync with graphics command list:
//        - Use smth like isUploadingDone? And then if it false, do not render it? Seems bad beacause then I need to check it for every
//          vertex/index buffers and every material texture.
//        - Assign some default resource and replace it with the real one when uploading is done.
//          But this way I should use mutex/lock? may be not
//    [LINKS]:
//      - https://docs.unity3d.com/2021.2/Documentation/Manual/LoadingTextureandMeshData.html
//
//  - <HeapPropertiesUnknown>
//    Why am I forced to use D3D12_CPU_PAGE_PROPERTY_UNKNOWN/D3D12_MEMORY_POOL_UNKNOWN in D3D12_HEAP_PROPERTIES?
//    What is the use of D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE/D3D12_MEMORY_POOL_L1 ?
//
//  - <RenderGraph>
//    Shaders loading happens after *Backend initialization, and in other Backends it's working, but not in this one.
//    I should have another step of initialization which will happen after Backend init, but before rendering starts.
//    And I think RenderGraph should solve this problem.
//
//  - <HDR>
//    [LINKS]:
//      - https://docs.microsoft.com/en-us/windows/win32/direct3darticles/high-dynamic-range
//
//  - <SplitBarriers>
//    Can I use split barriers(D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY/D3D12_RESOURCE_BARRIER_FLAG_END_ONLY) somewhere?
//    [LINKS]:
//      - https://docs.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12#split-barriers
//
//  - <RenderPasses>
//    Use ID3D12GraphicsCommandList4::BeginRenderPass(), EndRenderPass()
//    [LINKS]:
//      - https://docs.microsoft.com/en-us/windows/win32/direct3d12/direct3d-12-render-passes
// 
//  - <DX12RenderTexture>
//    - <SRV>
//      How to prevent access to SrvCpuDescriptor/SrvGpuDescriptor if RenderTexture will not be used in Pixel shader?
//      Put this pair int the std::optional ?
//    - <CurrentState>
//      Storing current resources state will not work with multithreaded command buffer recording.
//      But right know it doesn't matter, don't know how to manage transition barriers in RenderGraph differently anyway
//    - <Framebuffer>
//      I need to store DX12RenderTexture in DX12Framebuffer and in m_renderTextures map.
//      In OpenGL/DX11 they are stored as different entities, which is wrong, but workds. But in DX12/Vulkan I need to manage transition barriers
//        for render textures, so I need to have only one DX12RenderTexture entity.
//      I used std::shared_ptr to get things done quickly, but I'm not sure if this is the way, may be it would be better to store
//        DX12RenderTexture as RenderTextureHandle in the DX12Framebuffer or something.


// NOTE(v.matushkin): To enable D3D_FEATURE_LEVEL_12_2
//  https://devblogs.microsoft.com/directx/gettingstarted-dx12agility/#OS
//  https://www.nuget.org/packages/Microsoft.Direct3D.D3D12/1.4.10
// extern "C"
// {
//     __declspec(dllexport) extern const UINT D3D12SDKVersion = 4;
// }
// extern "C"
// {
//     __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\";
// }


static const D3D12_INPUT_ELEMENT_DESC k_InputElementDesc[] = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
};

// NOTE(v.matushkin): Questionable naming
static const D3D12_CLEAR_FLAGS dx12_DepthStencilClearFlag[] = {
    static_cast<D3D12_CLEAR_FLAGS>(0), // NOTE(v.matushkin): Hack
    D3D12_CLEAR_FLAG_DEPTH,
    D3D12_CLEAR_FLAG_STENCIL,
    D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
};

static const DXGI_FORMAT dx12_RenderTextureFormat[] = {
    DXGI_FORMAT_B8G8R8A8_UNORM,
    DXGI_FORMAT_D32_FLOAT,
};

// NOTE(v.matushkin): Same as in DX11Backend, place it in something like DXCommon.hpp?
// NOTE(v.matushkin): Don't know if I should use UINT or UNORM
// TODO(v.matushkin): Apparently there is no support fo 24 bit formats (like RGB8, DEPTH24) on all(almost?) gpus
//  So I guess, I should remove this formats from TextureGraphicsFormat
static const DXGI_FORMAT dx12_TextureFormat[] = {
    DXGI_FORMAT_R8_UINT,
    DXGI_FORMAT_R16_UINT,
    DXGI_FORMAT_R16_FLOAT,
    DXGI_FORMAT_R32_FLOAT,
    DXGI_FORMAT_R8G8_UINT,
    DXGI_FORMAT_R16G16_UINT,
    // DXGI_FORMAT_R8G8B8A8_UNORM,     // RGB8
    // DXGI_FORMAT_R16G16B16A16_UINT,  // RGB16
    DXGI_FORMAT_R8G8B8A8_UNORM,
    DXGI_FORMAT_R16G16B16A16_FLOAT,
    DXGI_FORMAT_D16_UNORM,
    // DXGI_FORMAT_D24_UNORM_S8_UINT,  // DEPTH24
    DXGI_FORMAT_D24_UNORM_S8_UINT,  // DEPTH32
    DXGI_FORMAT_D32_FLOAT
};

namespace RootParameterIndex
{
    static const ui32 cbPerFrame = 0;
    static const ui32 cbPerDraw  = 1;
    static const ui32 dtTextures = 2;
}
namespace ShaderRegister
{
    static const ui32 bPerFrame      = 0;
    static const ui32 bPerDraw       = 1;
    static const ui32 tBaseColorMap  = 0;
    static const ui32 sStatic        = 0;
}

static const DXGI_FORMAT k_SwapchainFormat          = DXGI_FORMAT_B8G8R8A8_UNORM;
// TODO(v.matushkin): k_EngineColorFormat and k_EngineDepthStencilFormat are for EngineRenderPass render textures
//  how am I supposed to remove this hardcoded shit? Somehow deduce this from shader(don't think this is possible) ?
//  Specify it in the shader? How does Unity handle this, what if I use same shader with different render texture formats.
static const DXGI_FORMAT k_EngineColorFormat        = DXGI_FORMAT_B8G8R8A8_UNORM;
static const DXGI_FORMAT k_EngineDepthStencilFormat = DXGI_FORMAT_D32_FLOAT;

// TODO(v.matushkin): <RenderGraph>
static bool g_IsPipelineInitialized = false;

static ui32 g_BufferHandleWorkaround        = 0;
static ui32 g_FramebufferHandleWorkaround   = 0;
static ui32 g_RenderTextureHandleWorkaround = 0;
static ui32 g_ShaderHandleWorkaround        = 0;
static ui32 g_TextureHandleWorkaround       = 0;


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

D3D12_RESOURCE_BARRIER ResourceTransition(ID3D12Resource2* resource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter)
{
    D3D12_RESOURCE_TRANSITION_BARRIER d3dResourceTransitionBarrier = {
        .pResource   = resource,
        .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, // ???
        .StateBefore = stateBefore,
        .StateAfter  = stateAfter,
    };

    return D3D12_RESOURCE_BARRIER{
        .Type       = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        .Flags      = D3D12_RESOURCE_BARRIER_FLAG_NONE, // ???
        .Transition = d3dResourceTransitionBarrier
    };
}


namespace snv
{

DX12Backend::DX12Backend()
    : m_shaderCompiler(std::make_unique<DX12ShaderCompiler>())
{
    CreateDevice();

    m_descriptorHeap = std::make_unique<DX12DescriptorHeap>(m_device.Get());

    CreateCommandQueue();
    CreateSwapchain();
    CreateCommandAllocators();
    CreateCommandList();
    CreateFence();

    CreateConstantBuffer(m_cbPerFrame.GetAddressOf(), sizeof(PerFrame) * k_BackBufferFrames);
    CreateConstantBuffer(m_cbPerDraw.GetAddressOf(), sizeof(PerDraw) * k_BackBufferFrames);

    CreateRootSignature();

    // TODO(v.matushkin): Viewport and SwapChain should be the same size?
    const auto& graphicsSettings = EngineSettings::GraphicsSettings;
    m_viewport = {
        .TopLeftX = 0,
        .TopLeftY = 0,
        .Width    = static_cast<f32>(graphicsSettings.RenderWidth),
        .Height   = static_cast<f32>(graphicsSettings.RenderHeight),
        .MinDepth = 0, // corresponds to D3D12_MIN_DEPTH
        .MaxDepth = 1, // corresponds to D3D12_MAX_DEPTH
    };
    m_scissorRect = {
        .left   = 0,
        .top    = 0,
        .right  = LONG_MAX,
        .bottom = LONG_MAX,
    };

    m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
}

DX12Backend::~DX12Backend()
{
    CloseHandle(m_fenceEvent);
}


void DX12Backend::EnableBlend()
{}

void DX12Backend::EnableDepthTest()
{}


void* DX12Backend::GetNativeRenderTexture(RenderTextureHandle renderTextureHandle)
{
    return (void*) m_renderTextures[renderTextureHandle]->SrvGpuDescriptor.ptr;
}


void DX12Backend::SetBlendFunction(BlendMode source, BlendMode destination)
{}

void DX12Backend::SetClearColor(f32 r, f32 g, f32 b, f32 a)
{
    m_clearColor[0] = r;
    m_clearColor[1] = g;
    m_clearColor[2] = b;
    m_clearColor[3] = a;
}

void DX12Backend::SetDepthFunction(DepthCompareFunction depthCompareFunction)
{}

void DX12Backend::SetViewport(i32 x, i32 y, i32 width, i32 height)
{}


void DX12Backend::Clear(BufferBit bufferBitMask)
{}


void DX12Backend::BeginFrame(const glm::mat4x4& localToWorld, const glm::mat4x4& cameraView, const glm::mat4x4& cameraProjection)
{
    // TODO(v.matushkin): <RenderGraph>
    if (g_IsPipelineInitialized == false)
    {
        g_IsPipelineInitialized = true;
        CreatePipeline();
    }

    //- Reset CommandAllocator and CommandList
    // Command list allocators can only be reset when the associated command lists have finished execution on the GPU
    auto commandAllocator = m_commandAllocators[m_currentBackBufferIndex].Get();
    commandAllocator->Reset();
    m_graphicsCommandList->Reset(commandAllocator, m_graphicsPipeline.Get());

    //- Set RootSignature ConstantBuffer's
    m_graphicsCommandList->SetGraphicsRootSignature(m_rootSignature.Get());
    {
        m_cbPerFrameData._CameraProjection = cameraProjection;
        m_cbPerFrameData._CameraView       = cameraView;
        m_cbPerDrawData._ObjectToWorld     = localToWorld;

        const auto cbPerFrameStartByte = m_currentBackBufferIndex * sizeof(PerFrame);
        const auto cbPerDrawStartByte  = m_currentBackBufferIndex * sizeof(PerDraw);
        D3D12_RANGE d3dReadRange = { .Begin = 0, .End = 0 };
        ui8* cbDataBegin;

        m_cbPerFrame->Map(0, &d3dReadRange, reinterpret_cast<void**>(&cbDataBegin));
        std::memcpy(&cbDataBegin[cbPerFrameStartByte], &m_cbPerFrameData, sizeof(PerFrame));
        m_cbPerFrame->Unmap(0, nullptr);

        m_cbPerDraw->Map(0, &d3dReadRange, reinterpret_cast<void**>(&cbDataBegin));
        std::memcpy(&cbDataBegin[cbPerDrawStartByte], &m_cbPerDrawData, sizeof(PerDraw));
        m_cbPerDraw->Unmap(0, nullptr);

        const auto cbPerFrameLocation = m_cbPerFrame->GetGPUVirtualAddress() + cbPerFrameStartByte;
        const auto cbPerDrawLocation  = m_cbPerDraw->GetGPUVirtualAddress() + cbPerDrawStartByte;
        m_graphicsCommandList->SetGraphicsRootConstantBufferView(RootParameterIndex::cbPerFrame, cbPerFrameLocation);
        m_graphicsCommandList->SetGraphicsRootConstantBufferView(RootParameterIndex::cbPerDraw, cbPerDrawLocation);
    }

    //- Set DescriptorHeaps
    ID3D12DescriptorHeap* const descriptorHeaps[] = {m_descriptorHeap->GetSRVHeap()};
    m_graphicsCommandList->SetDescriptorHeaps(1, descriptorHeaps);

    //- Set Rasterizer Stage
    // NOTE(v.matushkin): No way to set this once? May be with bundles?
    m_graphicsCommandList->RSSetViewports(1, &m_viewport);
    m_graphicsCommandList->RSSetScissorRects(1, &m_scissorRect);

    //- Set Input-Assembler Stage
    m_graphicsCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void DX12Backend::BeginRenderPass(FramebufferHandle framebufferHandle)
{
    auto& dx12Framebuffer = framebufferHandle == m_swapchainFramebufferHandle
                          ? m_swapchainFramebuffers[m_currentBackBufferIndex]
                          : m_framebuffers[framebufferHandle];

    const auto& dx12DepthStencilAttachment = dx12Framebuffer.DepthStencilAttachment;
    const auto  d3dDepthStencilClearFlags  = dx12Framebuffer.DepthStencilClearFlags;

    const auto* d3dDepthStencilDescriptor = d3dDepthStencilClearFlags != 0
                                          ? &dx12DepthStencilAttachment->Descriptor
                                          : nullptr;

    //- Resource transition barriers
    {
        std::vector<D3D12_RESOURCE_BARRIER> d3dResourceBarriers;

        for (auto& colorAttachment : dx12Framebuffer.ColorAttachments)
        {
            if (colorAttachment->CurrentState != D3D12_RESOURCE_STATE_RENDER_TARGET)
            {
                d3dResourceBarriers.push_back(ResourceTransition(
                    colorAttachment->Texture.Get(),
                    colorAttachment->CurrentState,
                    D3D12_RESOURCE_STATE_RENDER_TARGET)
                );
                colorAttachment->CurrentState = D3D12_RESOURCE_STATE_RENDER_TARGET;
            }
        }

        if (d3dResourceBarriers.size() > 0)
        {
            m_graphicsCommandList->ResourceBarrier(d3dResourceBarriers.size(), d3dResourceBarriers.data());
        }
    }
    //- Set RenderTargets
    {
        const auto& d3dColorDescriptors = dx12Framebuffer.ColorDescriptors;
        // NOTE(v.matushkin): RTsSingleHandleToDescriptorRange=true for multiple render targets?
        m_graphicsCommandList->OMSetRenderTargets(d3dColorDescriptors.size(), d3dColorDescriptors.data(), false, d3dDepthStencilDescriptor);
    }
    //- Clear Color attachments
    for (const auto& colorAttachment : dx12Framebuffer.ColorAttachments)
    {
        if (colorAttachment->LoadAction == RenderTextureLoadAction::Clear)
        {
            m_graphicsCommandList->ClearRenderTargetView(colorAttachment->Descriptor, colorAttachment->ClearValue.Color, 0, nullptr);
        }
    }
    //- Clear DepthStencil attachment
    if (d3dDepthStencilDescriptor != nullptr && dx12DepthStencilAttachment->LoadAction == RenderTextureLoadAction::Clear)
    {
        const auto depthStencilClearValue = dx12DepthStencilAttachment->ClearValue.DepthStencil;
        m_graphicsCommandList->ClearDepthStencilView(
            *d3dDepthStencilDescriptor,
            d3dDepthStencilClearFlags,
            depthStencilClearValue.Depth,
            depthStencilClearValue.Stencil,
            0,
            nullptr
        );
    }
}

void DX12Backend::BeginRenderPass(FramebufferHandle framebufferHandle, RenderTextureHandle input)
{
    auto& dx12InputRenderTexture = m_renderTextures[input];
    if (dx12InputRenderTexture->CurrentState != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
    {
        const auto d3dResourceBarrier = ResourceTransition(
            dx12InputRenderTexture->Texture.Get(),
            dx12InputRenderTexture->CurrentState,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        );
        m_graphicsCommandList->ResourceBarrier(1, &d3dResourceBarrier);

        dx12InputRenderTexture->CurrentState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    }

    BeginRenderPass(framebufferHandle);
}

void DX12Backend::EndFrame()
{
    //- Transition RenderTarget from RENDER_TARGET to PRESENT
    {
        auto& dx12SwapchainRenderTexture = m_swapchainFramebuffers[m_currentBackBufferIndex].ColorAttachments[0];
        SNV_ASSERT(dx12SwapchainRenderTexture->CurrentState == D3D12_RESOURCE_STATE_RENDER_TARGET, "That was an error");

        const auto d3dResourceBarrier = ResourceTransition(
            dx12SwapchainRenderTexture->Texture.Get(),
            dx12SwapchainRenderTexture->CurrentState,
            D3D12_RESOURCE_STATE_PRESENT
        );
        m_graphicsCommandList->ResourceBarrier(1, &d3dResourceBarrier);

        dx12SwapchainRenderTexture->CurrentState = D3D12_RESOURCE_STATE_PRESENT;
    }

    m_graphicsCommandList->Close();

    //- Execute CommandList
    ID3D12CommandList* commandLists[] = {m_graphicsCommandList.Get()};
    m_commandQueue->ExecuteCommandLists(1, commandLists);

    //- Present frame
    // TODO(v.matushkin): Present1() ?
    // NOTE(v.matushkin): <VSync>, <VSync/Tearing>
    m_swapChain->Present(0, 0);

    WaitForPreviousFrame();
}


// NOTE(v.matushkin): Useless vertexCount?
void DX12Backend::DrawBuffer(TextureHandle textureHandle, BufferHandle bufferHandle, i32 indexCount, i32 vertexCount)
{
    //- Set Index/Vertex buffers
    const auto& buffer = m_buffers[bufferHandle];
    D3D12_VERTEX_BUFFER_VIEW d3dVertexBuffers[] = {buffer.PositionView, buffer.NormalView, buffer.TexCoord0View};
    m_graphicsCommandList->IASetIndexBuffer(&buffer.IndexView);
    m_graphicsCommandList->IASetVertexBuffers(0, 3, d3dVertexBuffers);

    //- Set Material Texture
    const auto& texture = m_textures[textureHandle];
    m_graphicsCommandList->SetGraphicsRootDescriptorTable(RootParameterIndex::dtTextures, texture.GpuDescriptor);

    m_graphicsCommandList->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
}


IImGuiRenderContext* DX12Backend::CreateImGuiRenderContext()
{
    const auto [fontSrvCpuDescriptor, fontSrvGpuDescriptor] = m_descriptorHeap->AllocateSRV();
    return new DX12ImGuiRenderContext(
        m_graphicsCommandList.Get(),
        m_device.Get(),
        k_BackBufferFrames,
        k_SwapchainFormat,
        m_descriptorHeap->GetSRVHeap(),
        fontSrvCpuDescriptor,
        fontSrvGpuDescriptor
    );
}


GraphicsState DX12Backend::CreateGraphicsState(const GraphicsStateDesc& graphicsStateDesc)
{
    GraphicsState graphicsState;

    DX12Framebuffer dx12Framebuffer;

    //- Create Color attachments
    const auto& colorAttachments       = graphicsStateDesc.ColorAttachments;
    const auto  d3dRtvColorDescriptors = m_descriptorHeap->AllocateRTV(colorAttachments.size());

    for (ui32 i = 0; i < colorAttachments.size(); ++i)
    {
        const auto& colorDesc         = colorAttachments[i];
        const auto  dxgiColorFormat   = dx12_RenderTextureFormat[static_cast<ui8>(colorDesc.Format)];
        const auto  isUsageShaderRead = colorDesc.Usage == RenderTextureUsage::ShaderRead;

        ComPtr<ID3D12Resource2> d3dColorTexture;
        const auto              d3dRtvColorDescriptor = d3dRtvColorDescriptors[i];

        //-- Create Color texture
        // TODO(v.matushkin): <HeapPropertiesUnknown>
        D3D12_HEAP_PROPERTIES d3dHeapProperties = {
            .Type                 = D3D12_HEAP_TYPE_DEFAULT,
            .CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
            .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
            .CreationNodeMask     = 1,
            .VisibleNodeMask      = 1,
        };

        auto d3dResourceFlags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        if (isUsageShaderRead == false)
        {
            d3dResourceFlags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
        }
        D3D12_RESOURCE_DESC1 d3dResourceDesc = {
            .Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
            .Alignment        = 0,
            .Width            = colorDesc.Width,
            .Height           = colorDesc.Height,
            .DepthOrArraySize = 1,
            .MipLevels        = 0,
            .Format           = dxgiColorFormat,
            .SampleDesc       = {.Count = 1, .Quality = 0},
            .Layout           = D3D12_TEXTURE_LAYOUT_UNKNOWN,
            .Flags            = d3dResourceFlags,
            // .SamplerFeedbackMipRegion = ,
        };

        const auto* colorClearValue = colorDesc.ClearValue.Color;
        D3D12_CLEAR_VALUE d3dColorOptimizedClearValue = {
            .Format = dxgiColorFormat,
            .Color  = {colorClearValue[0], colorClearValue[1], colorClearValue[2], colorClearValue[3]},
        };

        m_device->CreateCommittedResource2(
            &d3dHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &d3dResourceDesc,
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            &d3dColorOptimizedClearValue,
            nullptr,
            IID_PPV_ARGS(d3dColorTexture.GetAddressOf())
        );

        //-- Create RenderTarget view
        // NOTE(v.matushkin): Why it works without setting Texture2D?
        D3D12_RENDER_TARGET_VIEW_DESC d3dRenderTargetViewDesc = {
            .Format        = dxgiColorFormat,
            .ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
            // .Texture2D     = ,
        };
        m_device->CreateRenderTargetView(d3dColorTexture.Get(), &d3dRenderTargetViewDesc, d3dRtvColorDescriptor);

        DX12RenderTexture dx12ColorAttachment = {
            .Texture      = d3dColorTexture,
            .Descriptor   = d3dRtvColorDescriptor,
            // .SrvCpuDescriptor = ,
            // .SrvGpuDescriptor = ,
            .CurrentState = D3D12_RESOURCE_STATE_RENDER_TARGET,
            .ClearValue   = colorDesc.ClearValue,
            .LoadAction   = colorDesc.LoadAction,
        };

        //-- Create Render Texture SRV
        if (isUsageShaderRead)
        {
            D3D12_TEX2D_SRV d3dTexture2DSrv = {
                // .MostDetailedMip     = ,
                .MipLevels = 1,
                // .PlaneSlice          = ,
                // .ResourceMinLODClamp = ,
            };
            D3D12_SHADER_RESOURCE_VIEW_DESC d3dRenderTargetSrvDesc = {
                .Format                  = dxgiColorFormat,
                .ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D,
                .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING, // NOTE(v.matushkin): texture channels
                                                                                     // swizzling, but when it's useful?
                .Texture2D               = d3dTexture2DSrv,
            };

            const auto [srvCpuDescriptor, srvGpuDescriptor] = m_descriptorHeap->AllocateSRV();
            dx12ColorAttachment.SrvCpuDescriptor            = srvCpuDescriptor;
            dx12ColorAttachment.SrvGpuDescriptor            = srvGpuDescriptor;

            m_device->CreateShaderResourceView(d3dColorTexture.Get(), &d3dRenderTargetSrvDesc, srvCpuDescriptor);
        }

        auto dx12ColorAttachmentPtr = std::make_shared<DX12RenderTexture>(dx12ColorAttachment);

        const auto renderTextureHandle        = static_cast<RenderTextureHandle>(g_RenderTextureHandleWorkaround++);
        m_renderTextures[renderTextureHandle] = dx12ColorAttachmentPtr;
        graphicsState.ColorAttachments.push_back(renderTextureHandle);

        dx12Framebuffer.ColorAttachments.push_back(std::move(dx12ColorAttachmentPtr));
        dx12Framebuffer.ColorDescriptors.push_back(d3dRtvColorDescriptor);
    }

    //- Create DepthStencil attachment
    dx12Framebuffer.DepthStencilClearFlags = dx12_DepthStencilClearFlag[static_cast<ui8>(graphicsStateDesc.DepthStencilType)];

    if (dx12Framebuffer.DepthStencilClearFlags != 0)
    {
        const auto& depthStencilDesc       = graphicsStateDesc.DepthStencilAttachment;
        const auto  depthStencilClearValue = depthStencilDesc.ClearValue.DepthStencil;
        const auto  dxgiDepthStencilFormat = dx12_RenderTextureFormat[static_cast<ui8>(depthStencilDesc.Format)];

        SNV_ASSERT(depthStencilDesc.Usage == RenderTextureUsage::Default, "Sampling DepthStencil texture is not supported");

        ComPtr<ID3D12Resource2> d3dDepthStencilTexture;
        const auto d3dDepthStencilDescriptor = m_descriptorHeap->AllocateDSV();

        //-- Create DepthStencil texture
        // TODO(v.matushkin): <HeapPropertiesUnknown>
        D3D12_HEAP_PROPERTIES d3dHeapProperties = {
            .Type                 = D3D12_HEAP_TYPE_DEFAULT,
            .CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
            .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
            .CreationNodeMask     = 1,
            .VisibleNodeMask      = 1,
        };
        // NOTE(v.matushkin): From PIX:
        //  These depth/stencil resources weren't used as shader resources during this capture, but the resources didn't have the
        //  D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE flag set on them at creation time. If the application doesn't use these
        //  resources as shader resources, then consider adding DENY_SHADER_RESOURCE to their creation flags to improve
        //  performance on some hardware. It will be particularly beneficial on AMD GCN 1.2+ hardware that supports "Delta Color
        //  Compression" (DCC).
        D3D12_RESOURCE_DESC1 d3dResourceDesc = {
            .Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
            .Alignment        = 0,
            .Width            = depthStencilDesc.Width,
            .Height           = depthStencilDesc.Height,
            .DepthOrArraySize = 1,
            .MipLevels        = 0,
            .Format           = dxgiDepthStencilFormat,
            .SampleDesc       = {.Count = 1, .Quality = 0},
            .Layout           = D3D12_TEXTURE_LAYOUT_UNKNOWN,
            .Flags            = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE,
            // .SamplerFeedbackMipRegion = ,
        };

        D3D12_DEPTH_STENCIL_VALUE d3dDepthStencilValue = {
            .Depth   = depthStencilClearValue.Depth,
            .Stencil = depthStencilClearValue.Stencil,
        };
        D3D12_CLEAR_VALUE d3dDepthOptimizedClearValue = {
            .Format       = dxgiDepthStencilFormat,
            .DepthStencil = d3dDepthStencilValue,
        };

        m_device->CreateCommittedResource2(
            &d3dHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &d3dResourceDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &d3dDepthOptimizedClearValue,
            nullptr,
            IID_PPV_ARGS(d3dDepthStencilTexture.GetAddressOf())
        );

        //-- Create DepthStencil view
        // NOTE(v.matushkin): Why it works without setting Texture2D?
        D3D12_DEPTH_STENCIL_VIEW_DESC d3dDepthStencilViewDesc = {
            .Format        = dxgiDepthStencilFormat,
            .ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
            .Flags         = D3D12_DSV_FLAG_NONE, // NOTE(v.matushkin): D3D12_DSV_FLAG_READ_ONLY_* ?
            // .Texture2D     = ,
        };
        m_device->CreateDepthStencilView(d3dDepthStencilTexture.Get(), &d3dDepthStencilViewDesc, d3dDepthStencilDescriptor);

        dx12Framebuffer.DepthStencilAttachment = std::make_shared<DX12RenderTexture>(DX12RenderTexture{
            .Texture      = d3dDepthStencilTexture,
            .Descriptor   = d3dDepthStencilDescriptor,
            // .SrvCpuDescriptor = ,
            // .SrvGpuDescriptor = ,
            .CurrentState = D3D12_RESOURCE_STATE_DEPTH_WRITE,
            .ClearValue   = depthStencilDesc.ClearValue,
            .LoadAction   = depthStencilDesc.LoadAction,
        });

        const auto renderTextureHandle        = static_cast<RenderTextureHandle>(g_RenderTextureHandleWorkaround++);
        graphicsState.DepthStencilAttachment  = renderTextureHandle;
        m_renderTextures[renderTextureHandle] = dx12Framebuffer.DepthStencilAttachment;
    }

    const auto framebufferHandle      = static_cast<FramebufferHandle>(g_FramebufferHandleWorkaround++);
    graphicsState.Framebuffer         = framebufferHandle;
    m_framebuffers[framebufferHandle] = std::move(dx12Framebuffer);

    return graphicsState;
}

// TODO(v.matushkin): Upload index buffer, better memory managment
BufferHandle DX12Backend::CreateBuffer(
    std::span<const std::byte>              indexData,
    std::span<const std::byte>              vertexData,
    const std::vector<VertexAttributeDesc>& vertexLayout
)
{
    DX12Buffer dx12Buffer;

    // TODO(v.matushkin): <HeapPropertiesUnknown>
    D3D12_HEAP_PROPERTIES d3dHeapProperties = {
        .Type                 = D3D12_HEAP_TYPE_UPLOAD,
        .CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
        .CreationNodeMask     = 1, // Multi-GPU
        .VisibleNodeMask      = 1, // Multi-GPU
    };
    DXGI_SAMPLE_DESC dxgiSampleDesc = {
        .Count   = 1,
        .Quality = 0,
    };
    D3D12_RESOURCE_DESC1 d3dResourceDesc = {
        .Dimension                = D3D12_RESOURCE_DIMENSION_BUFFER,
        .Alignment                = 0,
        // .Width                    = ,
        .Height                   = 1,
        .DepthOrArraySize         = 1,
        .MipLevels                = 1,
        .Format                   = DXGI_FORMAT_UNKNOWN,
        .SampleDesc               = dxgiSampleDesc,
        .Layout                   = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        .Flags                    = D3D12_RESOURCE_FLAG_NONE,
        // .SamplerFeedbackMipRegion = ,
    };
    // We do not intend to read from this resource on the CPU.
    D3D12_RANGE d3dRange = { .Begin = 0, .End = 0 };

    d3dResourceDesc.Width = indexData.size_bytes();
    m_device->CreateCommittedResource2(
        &d3dHeapProperties,
        D3D12_HEAP_FLAG_NONE,               // NOTE(v.matushkin): Sure?
        &d3dResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,  // NOTE(v.matushkin): Sure?
        nullptr,                            // D3D12_CLEAR_VALUE is null for buffers
        nullptr,                            // ID3D12ProtectedResourceSession1
        IID_PPV_ARGS(dx12Buffer.Index.GetAddressOf())
    );
    ui8* indexDataBegin;
    dx12Buffer.Index->Map(0, &d3dRange, reinterpret_cast<void**>(&indexDataBegin));
    std::memcpy(indexDataBegin, indexData.data(), indexData.size_bytes());
    dx12Buffer.Index->Unmap(0, nullptr);
    // Initialize the vertex buffer view.
    dx12Buffer.IndexView.BufferLocation = dx12Buffer.Index->GetGPUVirtualAddress();
    dx12Buffer.IndexView.SizeInBytes    = indexData.size_bytes();
    dx12Buffer.IndexView.Format         = DXGI_FORMAT_R32_UINT;

    ID3D12Resource2* d3dResources[] = {
        dx12Buffer.Position.Get(),
        dx12Buffer.Normal.Get(),
        dx12Buffer.TexCoord0.Get(),
    };
    D3D12_VERTEX_BUFFER_VIEW* d3dVertexBufferViews[] = {
        &dx12Buffer.PositionView,
        &dx12Buffer.NormalView,
        &dx12Buffer.TexCoord0View,
    };

    // TODO(v.matushkin): At least I should use one heap for this vertex buffers
    for (ui32 i = 0; i < vertexLayout.size(); ++i)
    {
        // TODO(v.matushkin): Adjust VertexAttributeDescriptor to remove this hacks
        const auto& vertexAttribute = vertexLayout[i];
        const auto  currOffset      = vertexAttribute.Offset;
        const auto  nextOffset      = (i + 1) < vertexLayout.size() ? vertexLayout[i + 1].Offset : vertexData.size_bytes();
        const auto  attributeSize   = (nextOffset - currOffset);

        d3dResourceDesc.Width = attributeSize;

        m_device->CreateCommittedResource2(
            &d3dHeapProperties,
            D3D12_HEAP_FLAG_NONE,               // NOTE(v.matushkin): Sure?
            &d3dResourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,  // NOTE(v.matushkin): Sure?
            nullptr,                            // D3D12_CLEAR_VALUE is null for buffers
            nullptr,                            // ID3D12ProtectedResourceSession1
            IID_PPV_ARGS(&d3dResources[i])
        );

        // Copy the triangle data to the vertex buffer.
        ui8* vertexDataBegin;
        d3dResources[i]->Map(0, &d3dRange, reinterpret_cast<void**>(&vertexDataBegin));
        std::memcpy(vertexDataBegin, vertexData.data() + currOffset, attributeSize);
        d3dResources[i]->Unmap(0, nullptr);
        // Initialize the vertex buffer view.
        d3dVertexBufferViews[i]->BufferLocation = d3dResources[i]->GetGPUVirtualAddress();
        d3dVertexBufferViews[i]->StrideInBytes  = sizeof(f32) * 3;
        d3dVertexBufferViews[i]->SizeInBytes    = attributeSize;
    }

    const auto bufferHandle = static_cast<BufferHandle>(g_BufferHandleWorkaround++);
    m_buffers[bufferHandle] = std::move(dx12Buffer);

    return bufferHandle;
}

TextureHandle DX12Backend::CreateTexture(const TextureDesc& textureDesc, const ui8* textureData)
{
    const auto dxgiTextureFormat = dx12_TextureFormat[static_cast<ui8>(textureDesc.Format)];

    DX12Texture dx12Texture;

    //- Create D3D12 TextureDesc
    DXGI_SAMPLE_DESC dxgiSampleDesc = {
        .Count   = 1,
        .Quality = 0,
    };
    D3D12_RESOURCE_DESC1 d3dResourceDesc = {
        .Dimension                = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        .Alignment                = 0,                               //NOTE(v.matushkin): ???
        .Width                    = textureDesc.Width,
        .Height                   = textureDesc.Height,
        .DepthOrArraySize         = 1,
        .MipLevels                = 1,
        .Format                   = dxgiTextureFormat,
        .SampleDesc               = dxgiSampleDesc,
        // .Layout                   = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,  //NOTE(v.matushkin): ???
        .Flags                    = D3D12_RESOURCE_FLAG_NONE,
        // .SamplerFeedbackMipRegion = ,
    };

    //- Create texture to the GPU
    // TODO(v.matushkin): <HeapPropertiesUnknown>
    D3D12_HEAP_PROPERTIES d3dHeapProperties = {
        .Type                 = D3D12_HEAP_TYPE_DEFAULT,
        .CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
        .CreationNodeMask     = 1, // Multi-GPU
        .VisibleNodeMask      = 1, // Multi-GPU
    };
    m_device->CreateCommittedResource2(
        &d3dHeapProperties,
        D3D12_HEAP_FLAG_NONE,               // NOTE(v.matushkin): Sure?
        &d3dResourceDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,                            // D3D12_CLEAR_VALUE is null for buffers
        nullptr,                            // ID3D12ProtectedResourceSession1
        IID_PPV_ARGS(dx12Texture.Texture.GetAddressOf())
    );

    //- Create CPU -> GPU upload buffer
    ui64 uploadBufferSize;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT d3dTextureLayout;
    ui32 numRows;

    m_device->GetCopyableFootprints1(
        &d3dResourceDesc,
        0, 1, 0,
        &d3dTextureLayout,
        &numRows,
        nullptr,
        &uploadBufferSize
    );

    d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    d3dResourceDesc.Width     = uploadBufferSize;
    d3dResourceDesc.Height    = 1;
    d3dResourceDesc.Format    = DXGI_FORMAT_UNKNOWN;
    d3dResourceDesc.Layout    = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    d3dHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

    ComPtr<ID3D12Resource2> d3dTextureUploadHeap;
    m_device->CreateCommittedResource2(
        &d3dHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &d3dResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        nullptr,
        IID_PPV_ARGS(d3dTextureUploadHeap.GetAddressOf())
    );

    //- Copy texture to the GPU
    // TODO(v.matushkin): I can get RowPitch/SlicePitch from TextureDesc or from GetCopyableFootprints1, is there any difference?
    const auto rowPitch = d3dTextureLayout.Footprint.RowPitch;
    D3D12_SUBRESOURCE_DATA d3dSubresourceData = {
        .pData      = textureData,
        .RowPitch   = rowPitch,
        .SlicePitch = rowPitch * numRows,
    };

    //-- Copy texture to intermediate upload heap
    D3D12_RANGE d3dReadRange = { .Begin = 0, .End = 0 };
    ui8* uploadBufferBegin;
    // NOTE(v.matushkin): Can pass nullptr instead of d3dReadRange, is it the same thing?
    d3dTextureUploadHeap->Map(0, &d3dReadRange, reinterpret_cast<void**>(&uploadBufferBegin));
    std::memcpy(uploadBufferBegin, textureData, uploadBufferSize);
    // NOTE(v.matushkin): Does it needs to be Unmapped?
    d3dTextureUploadHeap->Unmap(0, nullptr);

    //-- Copy texture from upload heap to GPU
    D3D12_TEXTURE_COPY_LOCATION d3dTextureCopyLocationSrc = {
        .pResource       = d3dTextureUploadHeap.Get(),
        .Type            = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
        .PlacedFootprint = d3dTextureLayout
    };
    D3D12_TEXTURE_COPY_LOCATION d3dTextureCopyLocationDst = {
        .pResource        = dx12Texture.Texture.Get(),
        .Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
        .SubresourceIndex = 0
    };

    const auto d3dResourceBarrier = ResourceTransition(
        dx12Texture.Texture.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );

    auto commandAllocator = m_commandAllocators[0].Get();
    commandAllocator->Reset();
    m_graphicsCommandList->Reset(commandAllocator, nullptr);
    // NOTE(v.matushkin): The fuck is DstX/DstY/DstZ and pSrcBox ?
    m_graphicsCommandList->CopyTextureRegion(&d3dTextureCopyLocationDst, 0, 0, 0, &d3dTextureCopyLocationSrc, nullptr);
    m_graphicsCommandList->ResourceBarrier(1, &d3dResourceBarrier);
    m_graphicsCommandList->Close();

    ID3D12CommandList* commandLists[] = {m_graphicsCommandList.Get()};
    m_commandQueue->ExecuteCommandLists(1, commandLists);

    //- Create Texture SRV
    D3D12_TEX2D_SRV d3dTexture2DSRV = {
        // .MostDetailedMip     = ,
        .MipLevels           = 1,
        // .PlaneSlice          = ,
        // .ResourceMinLODClamp = ,
    };
    D3D12_SHADER_RESOURCE_VIEW_DESC d3dTextureSRVDesc = {
        .Format                  = dxgiTextureFormat,
        .ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D,
        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING, // NOTE(v.matushkin): texture channels swizzling, but when it's useful?
        .Texture2D               = d3dTexture2DSRV,
    };

    const auto [srvCpuDescriptor, srvGpuDescriptor] = m_descriptorHeap->AllocateSRV();
    dx12Texture.CpuDescriptor                       = srvCpuDescriptor;
    dx12Texture.GpuDescriptor                       = srvGpuDescriptor;
    m_device->CreateShaderResourceView(dx12Texture.Texture.Get(), &d3dTextureSRVDesc, srvCpuDescriptor);

    const auto textureHandle  = static_cast<TextureHandle>(g_TextureHandleWorkaround++);
    m_textures[textureHandle] = std::move(dx12Texture);

    WaitForPreviousFrame();

    return textureHandle;
}

ShaderHandle DX12Backend::CreateShader(const ShaderDesc& shaderDesc)
{
    DX12Shader dx12Shader = {
        .VertexShader   = m_shaderCompiler->CompileShader(L"vs_6_5", shaderDesc.VertexSource),
        .FragmentShader = m_shaderCompiler->CompileShader(L"ps_6_5", shaderDesc.FragmentSource),
    };

    const auto shaderHandle = static_cast<ShaderHandle>(g_ShaderHandleWorkaround++);
    m_shaders[shaderHandle] = std::move(dx12Shader);

    return shaderHandle;
}


void DX12Backend::CreateDevice()
{
    ui32 dxgiFactoryFlags = 0;

#ifdef SNV_GPU_API_DEBUG_ENABLED
    dxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;

    ComPtr<ID3D12Debug3> d3dDebug;
    D3D12GetDebugInterface(IID_PPV_ARGS(d3dDebug.GetAddressOf()));
    d3dDebug->EnableDebugLayer();
    // hr = d3dDebug.As(&d3dDebug5);
    // NOTE(v.matushkin): Use SetEnableGPUBasedValidation() and
    //  SetEnableAutoDebugName() from ID3D12Debug5?
    //  Although for some fucking reason, ID3D12Debug3 is the max version I can get
    //  And apparently I should set resource names to ease the debug
#endif // SNV_GPU_API_DEBUG_ENABLED

    CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(m_factory.GetAddressOf()));

    ComPtr<IDXGIAdapter4> dxgiAdapter;
    const auto dxgiGPUPreference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
    // TODO(v.matushkin): Use IID_PPV_ARGS
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
    // ComPtr<ID3D12InfoQueue1> d3dInfoQueue;
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

void DX12Backend::CreateSwapchain()
{
    const auto& windowSettings = ApplicationSettings::WindowSettings;

    //- Create Swapchain
    // NOTE(v.matushkin): This member is valid only with bit-block transfer (bitblt) model swap chains.
    //  When using flip model swap chain, this member must be specified as {1, 0}
    DXGI_SAMPLE_DESC dxgiSwapChainSampleDesc = {
        .Count   = 1, // multisampling setting
        .Quality = 0, // vendor-specific flag
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
        .Width       = windowSettings.Width,
        .Height      = windowSettings.Height,
        .Format      = k_SwapchainFormat,               // TODO(v.matushkin): <SwapchainCreation/Format>
        .Stereo      = false,
        .SampleDesc  = dxgiSwapChainSampleDesc,
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = k_BackBufferFrames,
        .Scaling     = DXGI_SCALING_STRETCH,            // TODO(v.matushkin): Play with this
        .SwapEffect  = DXGI_SWAP_EFFECT_FLIP_DISCARD,
        .AlphaMode   = DXGI_ALPHA_MODE_UNSPECIFIED,     // NOTE(v.matushkin): Don't know
        .Flags       = dxgiSwapChainFlags,
    };

    const auto win32Window = Window::GetWin32Window();
    // NOTE(v.matushkin): Why? And why this called in CreateSwapChain() method? Shouldn't this be configurable?
    // Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen will be handled manually.
    m_factory->MakeWindowAssociation(win32Window, DXGI_MWA_NO_ALT_ENTER);

    ComPtr<IDXGISwapChain1> dxgiSwapChain;
    m_factory->CreateSwapChainForHwnd(
        m_commandQueue.Get(), // Swap chain needs the queue so that it can force a flush on it.
        win32Window,
        &dxgiSwapChainDesc,
        nullptr,
        nullptr,
        dxgiSwapChain.GetAddressOf()
    );
    dxgiSwapChain.As(&m_swapChain);

    //- Create Swapchain Framebuffer
    const auto d3dRtvColorDescriptors = m_descriptorHeap->AllocateRTV(k_BackBufferFrames);

    for (ui32 i = 0; i < k_BackBufferFrames; ++i)
    {
        ComPtr<ID3D12Resource2> d3dColorTexture;
        const auto              d3dRtvColorDescriptor = d3dRtvColorDescriptors[i];

        m_swapChain->GetBuffer(i, IID_PPV_ARGS(d3dColorTexture.GetAddressOf()));

        // NOTE(v.matushkin): D3D12_RENDER_TARGET_VIEW_DESC = null?
        m_device->CreateRenderTargetView(d3dColorTexture.Get(), nullptr, d3dRtvColorDescriptor);

        DX12RenderTexture dx12ColorAttachment = {
            .Texture      = d3dColorTexture,
            .Descriptor   = d3dRtvColorDescriptor,
            // .SrvCpuDescriptor = ,
            // .SrvGpuDescriptor = ,
            .CurrentState = D3D12_RESOURCE_STATE_PRESENT,
            .ClearValue   = {.Color = {0.0f, 0.0f, 0.0f, 0.0f}},
            .LoadAction   = RenderTextureLoadAction::Clear
        };

        m_swapchainFramebuffers[i] = {
            .ColorAttachments       = {std::make_shared<DX12RenderTexture>(dx12ColorAttachment)},
            .ColorDescriptors       = {d3dRtvColorDescriptor},
            // .DepthStencilAttachment = ,
            .DepthStencilClearFlags = dx12_DepthStencilClearFlag[0],
        };
    }

    m_swapchainFramebufferHandle = static_cast<FramebufferHandle>(g_FramebufferHandleWorkaround++);
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

void DX12Backend::CreateConstantBuffer(ID3D12Resource2** constantBuffer, ui32 size)
{
    // TODO(v.matushkin): <HeapPropertiesUnknown>
    D3D12_HEAP_PROPERTIES d3dHeapProperties = {
        .Type                 = D3D12_HEAP_TYPE_UPLOAD,
        .CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
        .CreationNodeMask     = 1, // Multi-GPU
        .VisibleNodeMask      = 1, // Multi-GPU
    };
    DXGI_SAMPLE_DESC dxgiSampleDesc = {
        .Count   = 1,
        .Quality = 0
    };
    D3D12_RESOURCE_DESC1 d3dResourceDesc = {
        .Dimension                = D3D12_RESOURCE_DIMENSION_BUFFER,
        .Alignment                = 0,
        .Width                    = size,
        .Height                   = 1,
        .DepthOrArraySize         = 1,
        .MipLevels                = 1,
        .Format                   = DXGI_FORMAT_UNKNOWN,
        .SampleDesc               = dxgiSampleDesc,
        .Layout                   = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        .Flags                    = D3D12_RESOURCE_FLAG_NONE,
        // .SamplerFeedbackMipRegion = ,
    };

    m_device->CreateCommittedResource2(
        &d3dHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &d3dResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        nullptr,
        IID_PPV_ARGS(constantBuffer)
    );
}

// TODO(v.matushkin): Root signature should be created based on shader reflection
void DX12Backend::CreateRootSignature()
{
    //- ConstantBuffer's
    // TODO(v.matushkin): The doc says that desctiptors must be sorted by frequency of change, so the most volatile should be first
    D3D12_ROOT_DESCRIPTOR1 d3dRootDescriptorPerFrame = {
        .ShaderRegister = ShaderRegister::bPerFrame,
        .RegisterSpace  = 0,
        .Flags          = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, // TODO(v.matushkin): Not sure what should I set for PerFrame/PerDraw
    };
    D3D12_ROOT_DESCRIPTOR1 d3dRootDescriptorPerDraw = {
        .ShaderRegister = ShaderRegister::bPerDraw,
        .RegisterSpace  = 0,
        .Flags          = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, // TODO(v.matushkin): Not sure what should I set for PerFrame/PerDraw
    };

    //- Material Texture
    D3D12_DESCRIPTOR_RANGE1 d3dTexturesDescriptorRange = {
        .RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
        .NumDescriptors                    = 1,
        .BaseShaderRegister                = ShaderRegister::tBaseColorMap,
        .RegisterSpace                     = 0,
        .Flags                             = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE, // TODO(v.matushkin): What should I use?
        .OffsetInDescriptorsFromTableStart = 0, // NOTE(v.matushkin): Don't know
    };
    D3D12_ROOT_DESCRIPTOR_TABLE1 d3dTexturesRootDescriptorTable = {
        .NumDescriptorRanges = 1,
        .pDescriptorRanges   = &d3dTexturesDescriptorRange,
    };

    D3D12_ROOT_PARAMETER1 d3dRootParameters[] = {
        // PerFrame
        {
            .ParameterType    = D3D12_ROOT_PARAMETER_TYPE_CBV,
            .Descriptor       = d3dRootDescriptorPerFrame,
            .ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX,
        },
        // PerDraw
        {
            .ParameterType    = D3D12_ROOT_PARAMETER_TYPE_CBV,
            .Descriptor       = d3dRootDescriptorPerDraw,
            .ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX,
        },
        // Material Texture
        {
            .ParameterType    = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
            .DescriptorTable  = d3dTexturesRootDescriptorTable,
            .ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL,
        }
    };

    D3D12_STATIC_SAMPLER_DESC d3dStaticSampler = {
        .Filter           = D3D12_FILTER_MIN_MAG_MIP_POINT,
        .AddressU         = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        .AddressV         = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        .AddressW         = D3D12_TEXTURE_ADDRESS_MODE_WRAP, // NOTE(v.matushkin): How to handle this for 2D textures?
        .MipLODBias       = 0,
        .MaxAnisotropy    = 0, // NOTE(v.matushkin): Only used for ANISOTROPIC filter?
        .ComparisonFunc   = D3D12_COMPARISON_FUNC_NEVER,
        // .BorderColor      = , // NOTE(v.matushkin): Only used if D3D11_TEXTURE_ADDRESS_BORDER is specified in Address*
        .MinLOD           = 0,
        .MaxLOD           = D3D12_FLOAT32_MAX,
        .ShaderRegister   = ShaderRegister::sStatic,
        .RegisterSpace    = 0,
        .ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL,
    };

    D3D12_ROOT_SIGNATURE_FLAGS d3dRootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
        | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
        | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
        | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
        | D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS
        | D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;

    D3D12_ROOT_SIGNATURE_DESC1 d3dRootSignatureDesc = {
        .NumParameters     = ARRAYSIZE(d3dRootParameters),
        .pParameters       = d3dRootParameters,
        .NumStaticSamplers = 1,
        .pStaticSamplers   = &d3dStaticSampler,
        .Flags             = d3dRootSignatureFlags,
    };
    D3D12_VERSIONED_ROOT_SIGNATURE_DESC d3dVersionedRootSignatureDesc = {
        .Version  = D3D_ROOT_SIGNATURE_VERSION_1_1,
        .Desc_1_1 = d3dRootSignatureDesc,
    };

    ComPtr<ID3DBlob> rootSignature;
    ComPtr<ID3DBlob> rootSignatureError;
    auto hr = D3D12SerializeVersionedRootSignature(
        &d3dVersionedRootSignatureDesc,
        rootSignature.GetAddressOf(),
        rootSignatureError.GetAddressOf()
    );
    // TODO(v.matushkin): Remove error check from release build?
    if (FAILED(hr))
    {
        OutputDebugStringA((LPSTR)rootSignatureError->GetBufferPointer());
    }

    m_device->CreateRootSignature(
        0,
        rootSignature->GetBufferPointer(),
        rootSignature->GetBufferSize(),
        IID_PPV_ARGS(m_rootSignature.GetAddressOf())
    );
}

void DX12Backend::CreatePipeline()
{
    // TODO(v.matushkin): Shouldn't get shader like this, tmp workaround
    const auto& dx12Shader           = m_shaders.begin()->second;
    const auto& dx12VertexBytecode   = dx12Shader.VertexShader;
    const auto& dx12FragmentBytecode = dx12Shader.FragmentShader;

    D3D12_SHADER_BYTECODE d3dVertexShaderBytecode = {
        .pShaderBytecode = dx12VertexBytecode.Bytecode.get(),
        .BytecodeLength  = dx12VertexBytecode.Length,
    };
    D3D12_SHADER_BYTECODE d3dFragmentShaderBytecode = {
        .pShaderBytecode = dx12FragmentBytecode.Bytecode.get(),
        .BytecodeLength  = dx12FragmentBytecode.Length,
    };

    D3D12_RENDER_TARGET_BLEND_DESC d3dRenderTargetBlendDesc = {
        .BlendEnable           = false,
        .LogicOpEnable         = false,
        .SrcBlend              = D3D12_BLEND_ONE,
        .DestBlend             = D3D12_BLEND_ZERO,
        .BlendOp               = D3D12_BLEND_OP_ADD,
        .SrcBlendAlpha         = D3D12_BLEND_ONE,
        .DestBlendAlpha        = D3D12_BLEND_ZERO,
        .BlendOpAlpha          = D3D12_BLEND_OP_ADD,
        .LogicOp               = D3D12_LOGIC_OP_NOOP,
        .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL,
    };
    D3D12_BLEND_DESC d3dBlendDesc = {
        .AlphaToCoverageEnable  = false,
        .IndependentBlendEnable = false,
    };
    for (ui32 i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    {
        d3dBlendDesc.RenderTarget[i] = d3dRenderTargetBlendDesc;
    }

    D3D12_RASTERIZER_DESC d3dRasterizerDesc = {
        .FillMode              = D3D12_FILL_MODE_SOLID,
        .CullMode              = D3D12_CULL_MODE_BACK,
        .FrontCounterClockwise = true,
        .DepthBias             = D3D12_DEFAULT_DEPTH_BIAS,
        .DepthBiasClamp        = D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
        .SlopeScaledDepthBias  = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
        .DepthClipEnable       = true,
        .MultisampleEnable     = false,
        .AntialiasedLineEnable = false,
        .ForcedSampleCount     = 0,
        .ConservativeRaster    = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,
    };
    D3D12_DEPTH_STENCIL_DESC d3dDepthStencilDesc = {
        .DepthEnable      = true,
        .DepthWriteMask   = D3D12_DEPTH_WRITE_MASK_ALL,
        .DepthFunc        = D3D12_COMPARISON_FUNC_LESS,
        .StencilEnable    = false,
        // .StencilReadMask  = , // ui8
        // .StencilWriteMask = , // ui8
        // .FrontFace        = , // D3D12_DEPTH_STENCILOP_DESC
        // .BackFace         = , // D3D12_DEPTH_STENCILOP_DESC
    };
    D3D12_INPUT_LAYOUT_DESC d3dInputLayoutDesc = {
        .pInputElementDescs = k_InputElementDesc,
        .NumElements        = ARRAYSIZE(k_InputElementDesc),
    };
    DXGI_SAMPLE_DESC dxgiSampleDesc = {
        .Count   = 1,
        .Quality = 0
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC d3dGraphicsPipelineDesc = {
        .pRootSignature        = m_rootSignature.Get(),
        .VS                    = d3dVertexShaderBytecode,
        .PS                    = d3dFragmentShaderBytecode,
        // .DS                    =,
        // .HS                    =,
        // .GS                    =,
        // .StreamOutput          = d3dStreamOutputDesc,
        .BlendState            = d3dBlendDesc,
        .SampleMask            = UINT_MAX,  // NOTE(v.matushkin): What is this?
        .RasterizerState       = d3dRasterizerDesc,
        .DepthStencilState     = d3dDepthStencilDesc,
        .InputLayout           = d3dInputLayoutDesc,
        // .IBStripCutValue       =,
        .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        .NumRenderTargets      = 1,
        // .RTVFormats            =,
        .DSVFormat             = k_EngineDepthStencilFormat,
        .SampleDesc            = dxgiSampleDesc,
        .NodeMask              = 0,
        // .CachedPSO             =,
        // .Flags                 =,
    };
    d3dGraphicsPipelineDesc.RTVFormats[0] = k_EngineColorFormat;

    m_device->CreateGraphicsPipelineState(&d3dGraphicsPipelineDesc, IID_PPV_ARGS(m_graphicsPipeline.GetAddressOf()));
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
