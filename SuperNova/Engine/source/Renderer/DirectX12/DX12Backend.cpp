#include <Engine/Renderer/DirectX12/DX12Backend.hpp>

#include <Engine/EngineSettings.hpp>
#include <Engine/Application/Window.hpp>
#include <Engine/Core/Assert.hpp>
#include <Engine/Renderer/RenderDefaults.hpp>
#include <Engine/Renderer/DirectX12/DX12DescriptorHeap.hpp>
#include <Engine/Renderer/DirectX12/DX12ImGuiRenderContext.hpp>
#include <Engine/Renderer/DirectX12/DX12ShaderCompiler.hpp>
#include <Engine/Renderer/GpuApiCommon/DXCommon.hpp>

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
//    Checked RenderPassTier, got 0 :( So idk if it's worth it? Probably should do it anyway
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


// #define DX12_NO_DEPTH_STENCIL 0
// #define DX12_NO_DEPTH_STENCIL_CLEAR_FLAG static_cast<D3D12_CLEAR_FLAGS>(DX12_NO_DEPTH_STENCIL)
// static const D3D12_CLEAR_FLAGS dx12_RenderTextureTypeToClearFlags[] = {
//     DX12_NO_DEPTH_STENCIL_CLEAR_FLAG,
//     D3D12_CLEAR_FLAG_DEPTH,
//     D3D12_CLEAR_FLAG_STENCIL,
//     D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
// };

// TODO(v.matushkin): Move this to DXCommon
static const DXGI_FORMAT dx12_RenderTextureFormat[] = {
    DXGI_FORMAT_B8G8R8A8_UNORM,
    DXGI_FORMAT_D32_FLOAT,
};

// TODO(v.matushkin): Move this to DXCommon
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
    DXGI_FORMAT_D32_FLOAT,
};

//- RenderPass

static constexpr D3D12_RESOURCE_STATES dx12_AttachmentLayout(snv::AttachmentLayout attachmentLayout, snv::RenderTextureType renderTextureType)
{
    if (attachmentLayout == snv::AttachmentLayout::Render)
    {
        return renderTextureType == snv::RenderTextureType::Color
                                 ? D3D12_RESOURCE_STATE_RENDER_TARGET
                                 : D3D12_RESOURCE_STATE_DEPTH_WRITE;
    }
    else if (attachmentLayout == snv::AttachmentLayout::ShaderSample)
    {
        return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    }

    return D3D12_RESOURCE_STATE_PRESENT;
}

static D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE dx12_AttachmentLoadAction(snv::AttachmentLoadAction attachmentLoadAction)
{
    static const D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE d3dAttachmentLoadAction[] = {
        D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE,
        D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR,
        D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD,
    };

    return d3dAttachmentLoadAction[static_cast<ui8>(attachmentLoadAction)];
}

static D3D12_RENDER_PASS_ENDING_ACCESS_TYPE dx12_AttachmentStoreAction(snv::AttachmentStoreAction attachmentStoreAction)
{
    static const D3D12_RENDER_PASS_ENDING_ACCESS_TYPE d3dAttachmentStoreAction[] = {
        D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE,
        D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD,
    };

    return d3dAttachmentStoreAction[static_cast<ui8>(attachmentStoreAction)];
}

//- Shader states

//-- RasterizerState
static D3D12_CULL_MODE dx12_CullMode(snv::CullMode cullMode)
{
    static const D3D12_CULL_MODE d3dCullMode[] = {
        D3D12_CULL_MODE_NONE,
        D3D12_CULL_MODE_FRONT,
        D3D12_CULL_MODE_BACK,
    };

    return d3dCullMode[static_cast<ui8>(cullMode)];
}

static D3D12_FILL_MODE dx12_PolygonMode(snv::PolygonMode polygonMode)
{
    static const D3D12_FILL_MODE d3dPolygonMode[] = {
        D3D12_FILL_MODE_SOLID,
        D3D12_FILL_MODE_WIREFRAME,
    };

    return d3dPolygonMode[static_cast<ui8>(polygonMode)];
}

//-- DepthStencilState
static D3D12_COMPARISON_FUNC dx12_CompareFunction(snv::CompareFunction compareFunction)
{
    static const D3D12_COMPARISON_FUNC d3dCompareFunction[] = {
        D3D12_COMPARISON_FUNC_NEVER,
        D3D12_COMPARISON_FUNC_LESS,
        D3D12_COMPARISON_FUNC_EQUAL,
        D3D12_COMPARISON_FUNC_LESS_EQUAL,
        D3D12_COMPARISON_FUNC_GREATER,
        D3D12_COMPARISON_FUNC_NOT_EQUAL,
        D3D12_COMPARISON_FUNC_GREATER_EQUAL,
        D3D12_COMPARISON_FUNC_ALWAYS,
    };

    return d3dCompareFunction[static_cast<ui8>(compareFunction)];
}

//-- BlendState
static D3D12_BLEND_OP dx12_BlendOp(snv::BlendOp blendOp)
{
    static const D3D12_BLEND_OP d3dBlendOp[] = {
        D3D12_BLEND_OP_ADD,
        D3D12_BLEND_OP_SUBTRACT,
        D3D12_BLEND_OP_REV_SUBTRACT,
        D3D12_BLEND_OP_MIN,
        D3D12_BLEND_OP_MAX,
    };

    return d3dBlendOp[static_cast<ui8>(blendOp)];
}

static D3D12_BLEND dx12_BlendFactor(snv::BlendFactor blendFactor)
{
    static const D3D12_BLEND d3dBlendFactor[] = {
        D3D12_BLEND_ZERO,
        D3D12_BLEND_ONE,
        D3D12_BLEND_SRC_COLOR,
        D3D12_BLEND_INV_SRC_COLOR,
        D3D12_BLEND_DEST_COLOR,
        D3D12_BLEND_INV_DEST_COLOR,
        D3D12_BLEND_SRC_ALPHA,
        D3D12_BLEND_INV_SRC_ALPHA,
        D3D12_BLEND_DEST_ALPHA,
        D3D12_BLEND_INV_DEST_ALPHA,
        D3D12_BLEND_SRC_ALPHA_SAT,
        D3D12_BLEND_SRC1_COLOR,
        D3D12_BLEND_INV_SRC1_COLOR,
        D3D12_BLEND_SRC1_ALPHA,
        D3D12_BLEND_INV_SRC1_ALPHA,
    };

    return d3dBlendFactor[static_cast<ui8>(blendFactor)];
}

static D3D12_LOGIC_OP dx12_BlendLogicOp(snv::BlendLogicOp blendLogicOp)
{
    static const D3D12_LOGIC_OP d3dBlendLogicOp[] = {
        D3D12_LOGIC_OP_CLEAR,
        D3D12_LOGIC_OP_SET,
        D3D12_LOGIC_OP_COPY,
        D3D12_LOGIC_OP_COPY_INVERTED,
        D3D12_LOGIC_OP_NOOP,
        D3D12_LOGIC_OP_INVERT,
        D3D12_LOGIC_OP_AND,
        D3D12_LOGIC_OP_NAND,
        D3D12_LOGIC_OP_OR,
        D3D12_LOGIC_OP_NOR,
        D3D12_LOGIC_OP_XOR,
        D3D12_LOGIC_OP_EQUIV,
        D3D12_LOGIC_OP_AND_REVERSE,
        D3D12_LOGIC_OP_AND_INVERTED,
        D3D12_LOGIC_OP_OR_REVERSE,
        D3D12_LOGIC_OP_OR_INVERTED,
    };

    return d3dBlendLogicOp[static_cast<ui8>(blendLogicOp)];
}


namespace RootParameterIndex
{
    static const ui32 cbPerFrame = 0;
    static const ui32 cbPerDraw  = 1;
    static const ui32 dtTextures = 2;
}
// b - constant buffer
// t - texture
// s - sampler
namespace ShaderRegister
{
    static const ui32 bPerFrame      = 0;
    static const ui32 bPerDraw       = 1;
    static const ui32 tBaseColorMap  = 0;
    static const ui32 sStatic        = 0;
}

// TODO(v.matushkin): k_EngineColorFormat and k_EngineDepthStencilFormat are for EngineRenderPass render textures
//  how am I supposed to remove this hardcoded shit? Somehow deduce this from shader(don't think this is possible) ?
//  Specify it in the shader? How does Unity handle this, what if I use same shader with different render texture formats.
static const DXGI_FORMAT k_EngineColorFormat        = DXGI_FORMAT_B8G8R8A8_UNORM;
static const DXGI_FORMAT k_EngineDepthStencilFormat = DXGI_FORMAT_D32_FLOAT;

static ui32 g_BufferHandleWorkaround        = 0;
static ui32 g_RenderPassHandleWorkaround    = 0;
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


void* DX12Backend::GetNativeRenderTexture(RenderTextureHandle renderTextureHandle)
{
    return (void*) m_renderTextures[renderTextureHandle]->SrvGpuDescriptor.ptr;
}


void DX12Backend::BeginFrame(const glm::mat4x4& localToWorld, const glm::mat4x4& cameraView, const glm::mat4x4& cameraProjection)
{
    //- Reset CommandAllocator and CommandList
    // Command list allocators can only be reset when the associated command lists have finished execution on the GPU
    auto commandAllocator = m_commandAllocators[m_currentBackBufferIndex].Get();
    commandAllocator->Reset();
    // NOTE(v.matushkin): Remember first binded pipeline in a frame and set it on Reset ?
    // m_graphicsCommandList->Reset(commandAllocator, m_graphicsPipeline.Get());
    m_graphicsCommandList->Reset(commandAllocator, nullptr);

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

void DX12Backend::BeginRenderPass(RenderPassHandle renderPassHandle)
{
    {
        RenderPassHandle actualRenderPassHandle;
        if (renderPassHandle == m_swapchainRenderPassHandle)
        {
            actualRenderPassHandle = static_cast<RenderPassHandle>(static_cast<ui32>(m_swapchainRenderPassHandle) + m_currentBackBufferIndex);
        }
        else
        {
            actualRenderPassHandle = renderPassHandle;
        }

        m_currentRenderPass = &m_renderPasses[actualRenderPassHandle];
    }

    //- InitialBarriers
    {
        const auto& renderPassBarriers = m_currentRenderPass->InitialBarriers;
        if (renderPassBarriers.size() != 0)
        {
            m_graphicsCommandList->ResourceBarrier(renderPassBarriers.size(), renderPassBarriers.data());
        }
    }
    //- BeginRenderPass
    {
        // NOTE(v.matushkin): No way of setting RTsSingleHandleToDescriptorRange now that OMSetRenderTargets is gone :(
        const auto& dx12Subpass = m_currentRenderPass->Subpass;
        m_graphicsCommandList->BeginRenderPass(
            dx12Subpass.ColorAttachments.size(),
            dx12Subpass.ColorAttachments.data(),
            dx12Subpass.DepthStencilAttachment.has_value() ? &dx12Subpass.DepthStencilAttachment.value() : nullptr,
            D3D12_RENDER_PASS_FLAG_NONE
        );
    }
}

void DX12Backend::BeginRenderPass(RenderPassHandle renderPassHandle, RenderTextureHandle input)
{
    // auto& dx12InputRenderTexture = m_renderTextures[input];
    // if (dx12InputRenderTexture->CurrentState != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
    // {
    //     const auto d3dResourceBarrier = ResourceTransition(
    //         dx12InputRenderTexture->Texture.Get(),
    //         dx12InputRenderTexture->CurrentState,
    //         D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    //     );
    //     m_graphicsCommandList->ResourceBarrier(1, &d3dResourceBarrier);
    // 
    //     dx12InputRenderTexture->CurrentState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    // }

    BeginRenderPass(renderPassHandle);
}

void DX12Backend::EndRenderPass()
{
    m_graphicsCommandList->EndRenderPass();
    //- FinalBarriers
    {
        const auto& renderPassBarriers = m_currentRenderPass->FinalBarriers;
        if (renderPassBarriers.size() != 0)
        {
            m_graphicsCommandList->ResourceBarrier(renderPassBarriers.size(), renderPassBarriers.data());
        }
    }
}

void DX12Backend::EndFrame()
{
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


void DX12Backend::BindShader(ShaderHandle shaderHandle)
{
    const auto& dx12Shader = m_shaders[shaderHandle];
    // NOTE(v.matushkin): SetPipelineState1?
    m_graphicsCommandList->SetPipelineState(dx12Shader.GraphicsPipeline.Get());
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
    const auto dxgiSwapchainFormat = dx12_RenderTextureFormat[static_cast<ui8>(EngineSettings::GraphicsSettings.SwapchainFormat)];
    return new DX12ImGuiRenderContext(
        m_graphicsCommandList.Get(),
        m_device.Get(),
        k_BackBufferFrames,
        dxgiSwapchainFormat,
        m_descriptorHeap->GetSRVHeap(),
        fontSrvCpuDescriptor,
        fontSrvGpuDescriptor
    );
}


RenderTextureHandle DX12Backend::CreateRenderTexture(const RenderTextureDesc& renderTextureDesc)
{
    const auto dxgiRenderTextureFormat = dx12_RenderTextureFormat[static_cast<ui8>(renderTextureDesc.Format)];
    const auto renderTextureType       = renderTextureDesc.RenderTextureType();
    const auto isColorRenderTexture    = renderTextureType == RenderTextureType::Color;
    const auto isUsageShaderRead       = renderTextureDesc.Usage == RenderTextureUsage::ShaderRead;

    //- Create D3D Texture
    ID3D12Resource2*            d3dTexture;
    D3D12_RESOURCE_STATES       d3dResourceInitialState;
    D3D12_CPU_DESCRIPTOR_HANDLE d3dRenderTargetDescriptor;
    D3D12_CLEAR_VALUE           d3dOptimizedClearValue;
    {
        // TODO(v.matushkin): <HeapPropertiesUnknown>
        D3D12_HEAP_PROPERTIES d3dHeapProperties = {
            .Type                 = D3D12_HEAP_TYPE_DEFAULT,
            .CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
            .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
            .CreationNodeMask     = 1,
            .VisibleNodeMask      = 1,
        };

        D3D12_RESOURCE_FLAGS d3dResourceFlags;
        d3dOptimizedClearValue.Format = dxgiRenderTextureFormat;

        if (isColorRenderTexture)
        {
            d3dResourceFlags          = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            d3dResourceInitialState   = D3D12_RESOURCE_STATE_RENDER_TARGET;
            d3dRenderTargetDescriptor = m_descriptorHeap->AllocateRTV();

            const auto* value     = renderTextureDesc.ClearValue.Color.Value;
            d3dOptimizedClearValue.Color[0] = value[0];
            d3dOptimizedClearValue.Color[1] = value[1];
            d3dOptimizedClearValue.Color[2] = value[2];
            d3dOptimizedClearValue.Color[3] = value[3];
        }
        else
        {
            d3dResourceFlags          = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
            d3dResourceInitialState   = D3D12_RESOURCE_STATE_DEPTH_WRITE;
            d3dRenderTargetDescriptor = m_descriptorHeap->AllocateDSV();

            const auto depthStencilClearValue = renderTextureDesc.ClearValue.DepthStencil;
            d3dOptimizedClearValue.DepthStencil = {
                .Depth   = depthStencilClearValue.Depth,
                .Stencil = depthStencilClearValue.Stencil,
            };
        }

        if (isUsageShaderRead == false)
        {
            d3dResourceFlags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
        }
        D3D12_RESOURCE_DESC1 d3dResourceDesc = {
            .Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
            .Alignment        = 0,
            .Width            = renderTextureDesc.Width,
            .Height           = renderTextureDesc.Height,
            .DepthOrArraySize = 1,
            .MipLevels        = 0,
            .Format           = dxgiRenderTextureFormat,
            .SampleDesc       = {.Count = 1, .Quality = 0},
            .Layout           = D3D12_TEXTURE_LAYOUT_UNKNOWN,
            .Flags            = d3dResourceFlags,
            // .SamplerFeedbackMipRegion = ,
        };
        // NOTE(v.matushkin): From PIX:
        //  These depth/stencil resources weren't used as shader resources during this capture, but the resources didn't have the
        //  D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE flag set on them at creation time. If the application doesn't use these
        //  resources as shader resources, then consider adding DENY_SHADER_RESOURCE to their creation flags to improve
        //  performance on some hardware. It will be particularly beneficial on AMD GCN 1.2+ hardware that supports "Delta Color
        //  Compression" (DCC).
        m_device->CreateCommittedResource2(
            &d3dHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &d3dResourceDesc,
            d3dResourceInitialState,
            &d3dOptimizedClearValue,
            nullptr,
            IID_PPV_ARGS(&d3dTexture)
        );
    }

    //- Create RTV or DSV
    if (isColorRenderTexture)
    {
        // NOTE(v.matushkin): Why it works without setting Texture2D?
        D3D12_RENDER_TARGET_VIEW_DESC d3dRenderTargetViewDesc = {
            .Format        = dxgiRenderTextureFormat,
            .ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
            // .Texture2D     = ,
        };
        m_device->CreateRenderTargetView(d3dTexture, &d3dRenderTargetViewDesc, d3dRenderTargetDescriptor);
    }
    else
    {
        // NOTE(v.matushkin): Why it works without setting Texture2D?
        D3D12_DEPTH_STENCIL_VIEW_DESC d3dDepthStencilViewDesc = {
            .Format        = dxgiRenderTextureFormat,
            .ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
            .Flags         = D3D12_DSV_FLAG_NONE, // NOTE(v.matushkin): D3D12_DSV_FLAG_READ_ONLY_* ?
            // .Texture2D     = ,
        };
        m_device->CreateDepthStencilView(d3dTexture, &d3dDepthStencilViewDesc, d3dRenderTargetDescriptor);
    }

    DX12RenderTexture dx12ColorAttachment = {
        .Texture    = d3dTexture,
        .Descriptor = d3dRenderTargetDescriptor,
        // .SrvCpuDescriptor = ,
        // .SrvGpuDescriptor = ,
        .ClearValue = d3dOptimizedClearValue,
        .Type       = renderTextureType,
    };

    //- Create SRV
    if (isUsageShaderRead)
    {
         D3D12_TEX2D_SRV d3dTexture2DSrv = {
             // .MostDetailedMip     = ,
             .MipLevels = 1,
             // .PlaneSlice          = ,
             // .ResourceMinLODClamp = ,
         };
         D3D12_SHADER_RESOURCE_VIEW_DESC d3dRenderTargetSrvDesc = {
             .Format                  = dxgiRenderTextureFormat,
             .ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D,
             .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING, // NOTE(v.matushkin): texture channels
                                                                                  // swizzling, but when it's useful?
             .Texture2D               = d3dTexture2DSrv,
         };

         const auto [srvCpuDescriptor, srvGpuDescriptor] = m_descriptorHeap->AllocateSRV();
         dx12ColorAttachment.SrvCpuDescriptor            = srvCpuDescriptor;
         dx12ColorAttachment.SrvGpuDescriptor            = srvGpuDescriptor;
    
         m_device->CreateShaderResourceView(dx12ColorAttachment.Texture.Get(), &d3dRenderTargetSrvDesc, srvCpuDescriptor);
    }

    const auto renderTextureHandle        = static_cast<RenderTextureHandle>(g_RenderTextureHandleWorkaround++);
    m_renderTextures[renderTextureHandle] = std::make_shared<DX12RenderTexture>(std::move(dx12ColorAttachment));

    return renderTextureHandle;
}

RenderPassHandle DX12Backend::CreateRenderPass(const RenderPassDesc& renderPassDesc)
{
    DX12RenderPass dx12RenderPass;

    // NOTE(v.matushkin): Retrieving same RenderTextures from m_renderTextures multiple times,
    //  may be get them once at the start?

    //- Barriers
    {
        for (const auto& colorAttachmentDesc : renderPassDesc.ColorAttachments)
        {
            const auto needInitialBarrier = colorAttachmentDesc.InitialLayout != AttachmentLayout::Render;
            const auto needFinalBarrier   = colorAttachmentDesc.FinalLayout != AttachmentLayout::Render;

            // NOTE(v.matushkin): Move this code to lambda? Doubt this is possible with constexpr usage
            if (needInitialBarrier || needFinalBarrier)
            {
                auto* d3dTexture = m_renderTextures[colorAttachmentDesc.RenderTextureHandle]->Texture.Get();

                if (needInitialBarrier)
                {
                    constexpr auto d3dFinalLayout = dx12_AttachmentLayout(AttachmentLayout::Render, RenderTextureType::Color);

                    dx12RenderPass.InitialBarriers.push_back(ResourceTransition(
                        d3dTexture,
                        dx12_AttachmentLayout(colorAttachmentDesc.InitialLayout, RenderTextureType::Color),
                        d3dFinalLayout
                    ));
                }
                if (needFinalBarrier)
                {
                    constexpr auto d3dInitialLayout = dx12_AttachmentLayout(AttachmentLayout::Render, RenderTextureType::Color);

                    dx12RenderPass.FinalBarriers.push_back(ResourceTransition(
                        d3dTexture,
                        d3dInitialLayout,
                        dx12_AttachmentLayout(colorAttachmentDesc.FinalLayout, RenderTextureType::Color)
                    ));
                }
            }
        }
        if (renderPassDesc.DepthStencilAttachment.has_value())
        {
            const auto depthStencilAttachmentDesc = renderPassDesc.DepthStencilAttachment.value();

            const auto needInitialBarrier = depthStencilAttachmentDesc.InitialLayout != AttachmentLayout::Render;
            const auto needFinalBarrier   = depthStencilAttachmentDesc.FinalLayout != AttachmentLayout::Render;

            if (needInitialBarrier || needFinalBarrier)
            {
                auto* d3dTexture = m_renderTextures[depthStencilAttachmentDesc.RenderTextureHandle]->Texture.Get();

                if (needInitialBarrier)
                {
                    // For D3D12_RESOURCE_STATES it doesn't matter if type is Depth/Stencil/DepthStencil
                    constexpr auto d3dFinalLayout = dx12_AttachmentLayout(AttachmentLayout::Render, RenderTextureType::Depth);

                    dx12RenderPass.InitialBarriers.push_back(ResourceTransition(
                        d3dTexture,
                        dx12_AttachmentLayout(depthStencilAttachmentDesc.InitialLayout, RenderTextureType::Depth),
                        d3dFinalLayout
                    ));
                }
                if (needFinalBarrier)
                {
                    constexpr auto d3dInitialLayout = dx12_AttachmentLayout(AttachmentLayout::Render, RenderTextureType::Depth);

                    dx12RenderPass.FinalBarriers.push_back(ResourceTransition(
                        d3dTexture,
                        d3dInitialLayout,
                        dx12_AttachmentLayout(depthStencilAttachmentDesc.FinalLayout, RenderTextureType::Depth)
                    ));
                }
            }
        }
    }

    //- Subpass
    {
        for (const auto& colorAttachmentDesc : renderPassDesc.ColorAttachments)
        {
            const auto* dx12RenderTexture = m_renderTextures[colorAttachmentDesc.RenderTextureHandle].get();

            dx12RenderPass.Subpass.ColorAttachments.push_back(D3D12_RENDER_PASS_RENDER_TARGET_DESC{
                .cpuDescriptor   = dx12RenderTexture->Descriptor,
                .BeginningAccess = {
                    .Type  = dx12_AttachmentLoadAction(colorAttachmentDesc.LoadAction),
                    .Clear = dx12RenderTexture->ClearValue,
                },
                .EndingAccess = {
                    .Type = dx12_AttachmentStoreAction(colorAttachmentDesc.StoreAction),
                    // .Resolve = ,
                },
            });
        }
        if (renderPassDesc.DepthStencilAttachment.has_value())
        {
            // NOTE(v.matushkin): I copy AttachmentDesc here but for color attachments a take it by reference
            //  Which one is better?
            const auto  depthStencilAttachmentDesc = renderPassDesc.DepthStencilAttachment.value();
            const auto* dx12RenderTexture = m_renderTextures[depthStencilAttachmentDesc.RenderTextureHandle].get();

            dx12RenderPass.Subpass.DepthStencilAttachment = D3D12_RENDER_PASS_DEPTH_STENCIL_DESC{
                .cpuDescriptor = dx12RenderTexture->Descriptor,
                .DepthBeginningAccess = {
                    .Type  = dx12_AttachmentLoadAction(depthStencilAttachmentDesc.LoadAction),
                    .Clear = dx12RenderTexture->ClearValue,
                },
                .DepthEndingAccess = {
                    .Type = dx12_AttachmentStoreAction(depthStencilAttachmentDesc.StoreAction),
                    // .Resolve = ,
                },
                // .StencilBeginningAccess = ,
                // .StencilEndingAccess = ,
            };
        }
    }

    const auto renderPassHandle      = static_cast<RenderPassHandle>(g_RenderPassHandleWorkaround++);
    m_renderPasses[renderPassHandle] = std::move(dx12RenderPass);

    return renderPassHandle;
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
    // NOTE(v.matushkin): How to handle shader compilation errors?
    // NOTE(v.matushkin): Not sure about creating this class instance every time I need to compile a shader,
    //  but keeping this through all programm lifetime seems wrong too.
    //  At least there should be a way to compile a bunch of shaders at once.
    const auto dx12ShaderCompiler = DX12ShaderCompiler(shaderDesc);
    const auto isImGuiShader      = shaderDesc.IsImGuiShader();

    //- InputLayout
    const auto d3dInputElementsDesc = dx12ShaderCompiler.GetInputLayoutDesc();
    D3D12_INPUT_LAYOUT_DESC d3dInputLayoutDesc = {
        .pInputElementDescs = d3dInputElementsDesc.data(),
        .NumElements        = static_cast<ui32>(d3dInputElementsDesc.size()),
    };

    //- RasterizerState
    const auto rasterizerStateDesc = shaderDesc.RasterizerStateDesc;

    D3D12_RASTERIZER_DESC d3dRasterizerDesc = {
        .FillMode              = dx12_PolygonMode(rasterizerStateDesc.PolygonMode),
        .CullMode              = dx12_CullMode(rasterizerStateDesc.CullMode),
        .FrontCounterClockwise = dx_TriangleFrontFace(rasterizerStateDesc.FrontFace, isImGuiShader),
        .DepthBias             = D3D12_DEFAULT_DEPTH_BIAS,
        .DepthBiasClamp        = D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
        .SlopeScaledDepthBias  = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
        .DepthClipEnable       = true,
        .MultisampleEnable     = false,
        .AntialiasedLineEnable = false,
        .ForcedSampleCount     = 0,
        .ConservativeRaster    = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,
    };

    //- DepthStencilState
    const auto depthStencilStateDesc = shaderDesc.DepthStencilStateDesc;

    D3D12_DEPTH_STENCILOP_DESC d3dDepthStencilOpDesc = {
        .StencilFailOp      = D3D12_STENCIL_OP_KEEP,
        .StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
        .StencilPassOp      = D3D12_STENCIL_OP_KEEP,
        .StencilFunc        = D3D12_COMPARISON_FUNC_ALWAYS,
    };
    D3D12_DEPTH_STENCIL_DESC d3dDepthStencilDesc = {
        .DepthEnable      = depthStencilStateDesc.DepthTestEnable,
        .DepthWriteMask   = depthStencilStateDesc.DepthWriteEnable ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO,
        .DepthFunc        = dx12_CompareFunction(depthStencilStateDesc.DepthCompareFunction),
        .StencilEnable    = false,
        .StencilReadMask  = D3D12_DEFAULT_STENCIL_READ_MASK,
        .StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK,
        .FrontFace        = d3dDepthStencilOpDesc,
        .BackFace         = d3dDepthStencilOpDesc,
    };

    //- BlendState
    const auto blendStateDesc = shaderDesc.BlendStateDesc;

    D3D12_RENDER_TARGET_BLEND_DESC d3dRenderTargetBlendDesc = {
        // NOTE(v.matushkin): May be there is no need to set this if BlendMode::Off ?
        .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL,
    };

    if (blendStateDesc.BlendMode == BlendMode::BlendOp)
    {
        d3dRenderTargetBlendDesc.BlendEnable    = true;
        d3dRenderTargetBlendDesc.SrcBlend       = dx12_BlendFactor(blendStateDesc.ColorSrcBlendFactor);
        d3dRenderTargetBlendDesc.DestBlend      = dx12_BlendFactor(blendStateDesc.ColorDstBlendFactor);
        d3dRenderTargetBlendDesc.BlendOp        = dx12_BlendOp(blendStateDesc.ColorBlendOp);
        d3dRenderTargetBlendDesc.SrcBlendAlpha  = dx12_BlendFactor(blendStateDesc.AlphaSrcBlendFactor);
        d3dRenderTargetBlendDesc.DestBlendAlpha = dx12_BlendFactor(blendStateDesc.AlphaDstBlendFactor);
        d3dRenderTargetBlendDesc.BlendOpAlpha   = dx12_BlendOp(blendStateDesc.AlphaBlendOp);
    }
    else if (blendStateDesc.BlendMode == BlendMode::LogicOp)
    {
        d3dRenderTargetBlendDesc.LogicOpEnable = true;
        d3dRenderTargetBlendDesc.LogicOp       = dx12_BlendLogicOp(blendStateDesc.LogicOp);
    }

    D3D12_BLEND_DESC d3dBlendDesc = {
        .AlphaToCoverageEnable  = false,
        .IndependentBlendEnable = RenderDefaults::IndependentBlendEnable,
    };
    // TODO(v.matushkin): Get RenderTargets count from reflection and set only needed, not all?
    for (ui32 i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    {
        d3dBlendDesc.RenderTarget[i] = d3dRenderTargetBlendDesc;
    }

    // NOTE(v.matushkin): And who the fuck are you?
    DXGI_SAMPLE_DESC dxgiSampleDesc = {
        .Count   = 1,
        .Quality = 0
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC d3dGraphicsPipelineDesc = {
        .pRootSignature        = m_rootSignature.Get(),
        .VS                    = dx12ShaderCompiler.GetVertexShaderBuffer(),
        .PS                    = dx12ShaderCompiler.GetPixelShaderBuffer(),
        // .DS                    =,
        // .HS                    =,
        // .GS                    =,
        // .StreamOutput          =,
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
        // NOTE(v.matushkin): D3D12_PIPELINE_STATE_FLAG_TOOL_DEBUG can only be set on WARP devices
        .Flags                 = D3D12_PIPELINE_STATE_FLAG_NONE,
    };
    d3dGraphicsPipelineDesc.RTVFormats[0] = k_EngineColorFormat;

    ID3D12PipelineState* d3dGraphicsPipeline;
    m_device->CreateGraphicsPipelineState(&d3dGraphicsPipelineDesc, IID_PPV_ARGS(&d3dGraphicsPipeline));

    const auto shaderHandle = static_cast<ShaderHandle>(g_ShaderHandleWorkaround++);
    m_shaders[shaderHandle] = DX12Shader{.GraphicsPipeline = d3dGraphicsPipeline};

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
    const auto& windowSettings      = ApplicationSettings::WindowSettings;
    const auto  dxgiSwapchainFormat = dx12_RenderTextureFormat[static_cast<ui8>(EngineSettings::GraphicsSettings.SwapchainFormat)];

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
        .Format      = dxgiSwapchainFormat,             // TODO(v.matushkin): <SwapchainCreation/Format>
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

    //- Create Swapchain RenderTextures/RenderPasses
    m_swapchainRenderTextureHandle = static_cast<RenderTextureHandle>(g_RenderTextureHandleWorkaround);
    m_swapchainRenderPassHandle    = static_cast<RenderPassHandle>(g_RenderPassHandleWorkaround);

    const auto d3dRtvColorDescriptors = m_descriptorHeap->AllocateRTV(k_BackBufferFrames);

    for (ui32 i = 0; i < k_BackBufferFrames; ++i)
    {
        ID3D12Resource2* d3dColorTexture;
        const auto       d3dRtvColorDescriptor = d3dRtvColorDescriptors[i];

        m_swapChain->GetBuffer(i, IID_PPV_ARGS(&d3dColorTexture));

        // NOTE(v.matushkin): D3D12_RENDER_TARGET_VIEW_DESC = null?
        m_device->CreateRenderTargetView(d3dColorTexture, nullptr, d3dRtvColorDescriptor);

        DX12RenderTexture dx12ColorAttachment = {
            .Texture    = d3dColorTexture,
            .Descriptor = d3dRtvColorDescriptor,
            // .SrvCpuDescriptor = ,
            // .SrvGpuDescriptor = ,
            .ClearValue = {.Format = dxgiSwapchainFormat, .Color = {0.0f, 0.0f, 0.0f, 0.0f}},
            .Type       = RenderTextureType::Color,
        };

        const auto renderTextureHandle        = static_cast<RenderTextureHandle>(g_RenderTextureHandleWorkaround++);
        m_renderTextures[renderTextureHandle] = std::make_shared<DX12RenderTexture>(DX12RenderTexture{
            .Texture    = d3dColorTexture,
            .Descriptor = d3dRtvColorDescriptor,
            // .SrvCpuDescriptor = ,
            // .SrvGpuDescriptor = ,
            .ClearValue = {.Format = dxgiSwapchainFormat, .Color = {0.0f, 0.0f, 0.0f, 0.0f}},
            .Type       = RenderTextureType::Color,
        });

        CreateRenderPass(RenderPassDesc{
            .ColorAttachments = {
                {
                    .RenderTextureHandle = renderTextureHandle,
                    .LoadAction          = AttachmentLoadAction::Clear,
                    .StoreAction         = AttachmentStoreAction::Store,
                    .InitialLayout       = AttachmentLayout::Present,
                    .FinalLayout         = AttachmentLayout::Present,
                },
            },
            // .DepthStencilAttachment = ,
            .Subpass = {
                .ColorAttachmentIndices = {0},
                // .DepthStencilAttachmentIndex = ,
            },
        });
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

    // NOTE(v.matushkin): https://developer.nvidia.com/dx12-dos-and-donts#roots
    //  Don't simultaneously set visible and deny flags for the same shader stages on root table entries
    //  For current drivers the deny flags only work when D3D12_SHADER_VISIBILITY_ALL is set
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


// #undef DX12_NO_DEPTH_STENCIL_CLEAR_FLAG
// #undef DX12_NO_DEPTH_STENCIL
