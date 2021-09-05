#include <Renderer/DirectX12/DX12Backend.hpp>

#include <Core/Assert.hpp>
#include <Core/Window.hpp>

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
//        Right now I just memcpy texture data to UPLOAD(CPU) heap. Things will get more complicated with mipmaps, texture arrays.
//        [LINKS]:
//          - http://alextardif.com/D3D11To12P3.html
//          - https://mynameismjp.wordpress.com/2016/03/25/bindless-texturing-for-deferred-rendering-and-decals/
//
//  - <WindowSize>
//    Window size is hardcoded (k_WindowWidth/k_WindowHeight), pass this values in costructor, or get them from window (first one is better I think).
//
//  - <TextureSampler>
//    Right now CreateTexture ignores texture wrapping options, there is only one static sampler. I don't know how to manage differnt samplers
//
//  - <AsynResourceUpload>
//    Right now CreateTexture(and later CreateMesh) call is blocking until it's done uploading to the GPU.
//    There should be async resource uploading API, although right now I don't how I'm gonna synchronize it with the rendering
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
//    And I think RenderGraph should solve this problem


// TODO(v.matushkin): <WindowSize>
const ui32 k_WindowWidth  = 1100;
const ui32 k_WindowHeight = 800;

const D3D12_INPUT_ELEMENT_DESC k_InputElementDesc[] = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
};

// NOTE(v.matushkin): Same as in DX11Backend, place it in something like DXCommon.hpp?
// NOTE(v.matushkin): Don't know if I should use UINT or UNORM
// TODO(v.matushkin): Apparently there is no support fo 24 bit formats (like RGB8, DEPTH24) on all(almost?) gpus
//  So I guess, I should remove this formats from TextureGraphicsFormat
const DXGI_FORMAT dx12_TextureFormat[] = {
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
    const ui32 cbPerFrame = 0;
    const ui32 cbPerDraw  = 1;
    const ui32 dtTextures = 2;
}
namespace ShaderRegister
{
    const ui32 bPerFrame      = 0;
    const ui32 bPerDraw       = 1;
    const ui32 tTextureAlbedo = 0;
    const ui32 sStatic        = 0;
}

const DXGI_FORMAT k_DepthStencilFormat = DXGI_FORMAT_D32_FLOAT;
const f32 k_DepthClearValue = 1.0f;

// TODO(v.matushkin): <RenderGraph>
bool g_IsPipelineInitialized = false;

// TODO(v.matushkin): <DynamicTextureDescriptorHeap>
const ui32 k_MaxTextureDescriptors = 300;


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
    : m_shaderCompiler(std::make_unique<DX12ShaderCompiler>())
{
    CreateDevice();
    CreateCommandQueue();
    CreateSwapChain();
    CreateDescriptorHeaps();
    CreateRenderTargetViews();
    CreateDepthStencilView();
    CreateCommandAllocators();
    CreateCommandList();
    CreateFence();

    CreateConstantBuffer(m_cbPerFrame.GetAddressOf(), sizeof(PerFrame) * k_BackBufferFrames);
    CreateConstantBuffer(m_cbPerDraw.GetAddressOf(), sizeof(PerDraw) * k_BackBufferFrames);

    CreateRootSignature();

    // TODO(v.matushkin): <WindowSize> Viewport and SwapChain should be the same size?
    m_viewport = {
        .TopLeftX = 0,
        .TopLeftY = 0,
        .Width    = k_WindowWidth,
        .Height   = k_WindowHeight,
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
    // TODO(v.matushkin): <RenderGraph>
    if (g_IsPipelineInitialized == false)
    {
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
    ID3D12DescriptorHeap* descriptorHeaps[] = { m_descriptorHeapSRV.Get() };
    m_graphicsCommandList->SetDescriptorHeaps(1, descriptorHeaps);

    //- Set Rasterizer Stage
    // NOTE(v.matushkin): No way to set this once? May be with bundles?
    m_graphicsCommandList->RSSetViewports(1, &m_viewport);
    m_graphicsCommandList->RSSetScissorRects(1, &m_scissorRect);

    //- Transition RenderTarget from PRESENT to RENDER_TARGET
    D3D12_RESOURCE_TRANSITION_BARRIER d3dResourceTransitionBarrier = {
        .pResource   = m_backBuffers[m_currentBackBufferIndex].Get(),
        .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, // ???
        .StateBefore = D3D12_RESOURCE_STATE_PRESENT,
        .StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET,
    };
    D3D12_RESOURCE_BARRIER d3dResourceBarrier = {
        .Type       = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        .Flags      = D3D12_RESOURCE_BARRIER_FLAG_NONE, // ???
        .Transition = d3dResourceTransitionBarrier,
    };
    m_graphicsCommandList->ResourceBarrier(1, &d3dResourceBarrier);

    //- Set RenderTarget and DepthStencil
    auto descriptorHandleRTV = m_descriptorHeapRTV->GetCPUDescriptorHandleForHeapStart();
    descriptorHandleRTV.ptr += m_rtvDescriptorSize * m_currentBackBufferIndex;
    auto descriptorHandleDSV = m_descriptorHeapDSV->GetCPUDescriptorHandleForHeapStart();
    // NOTE(v.matushkin): RTsSingleHandleToDescriptorRange=true for multiple render targets?
    m_graphicsCommandList->OMSetRenderTargets(1, &descriptorHandleRTV, false, &descriptorHandleDSV);

    //- Clear RenderTarget and DepthStencil
    m_graphicsCommandList->ClearRenderTargetView(descriptorHandleRTV, m_clearColor, 0, nullptr);
    m_graphicsCommandList->ClearDepthStencilView(descriptorHandleDSV, D3D12_CLEAR_FLAG_DEPTH, k_DepthClearValue, 0, 0, nullptr);

    //- Set Input-Assembler Stage
    m_graphicsCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void DX12Backend::EndFrame()
{
    //- Transition RenderTarget from RENDER_TARGET to PRESENT
    D3D12_RESOURCE_TRANSITION_BARRIER d3dResourceTransitionBarrier = {
        .pResource   = m_backBuffers[m_currentBackBufferIndex].Get(),
        .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, // ???
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
    auto srvGPUDescriptorHandle = m_descriptorHeapSRV->GetGPUDescriptorHandleForHeapStart();
    srvGPUDescriptorHandle.ptr += m_srvDescriptorSize * texture.IndexInDescriptorHeap;
    m_graphicsCommandList->SetGraphicsRootDescriptorTable(RootParameterIndex::dtTextures, srvGPUDescriptorHandle);

    m_graphicsCommandList->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
}

void DX12Backend::DrawArrays(i32 count)
{}

void DX12Backend::DrawElements(i32 count)
{}


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

        auto hr = m_device->CreateCommittedResource2(
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

    static ui32 buffer_handle_workaround = 0;
    auto        graphicsBufferHandle     = static_cast<BufferHandle>(buffer_handle_workaround++);

    m_buffers[graphicsBufferHandle] = dx12Buffer;

    return graphicsBufferHandle;
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

    //- Create texture on the GPU
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

    Microsoft::WRL::ComPtr<ID3D12Resource2> d3dTextureUploadHeap;
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

    D3D12_RESOURCE_TRANSITION_BARRIER d3dResourceTransitionBarrier = {
        .pResource   = dx12Texture.Texture.Get(),
        .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, // NOTE(v.matushkin): ???
        .StateBefore = D3D12_RESOURCE_STATE_COPY_DEST,
        .StateAfter  = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
    };
    D3D12_RESOURCE_BARRIER d3dResourceBarrier = {
        .Type       = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        .Flags      = D3D12_RESOURCE_BARRIER_FLAG_NONE,
        .Transition = d3dResourceTransitionBarrier,
    };

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

    static ui32 texture_handle_workaround = 0;

    // TODO(v.matushkin): This shouldn't depend on texture_handle_workaround. One hack depends on the other one, nice!
    auto srvDescriptorHandle = m_descriptorHeapSRV->GetCPUDescriptorHandleForHeapStart();
    dx12Texture.IndexInDescriptorHeap = texture_handle_workaround;
    srvDescriptorHandle.ptr += m_srvDescriptorSize * texture_handle_workaround;
    m_device->CreateShaderResourceView(dx12Texture.Texture.Get(), &d3dTextureSRVDesc, srvDescriptorHandle);

    auto textureHandle = static_cast<TextureHandle>(texture_handle_workaround++);
    m_textures[textureHandle] = dx12Texture;

    WaitForPreviousFrame();

    return textureHandle;
}

ShaderHandle DX12Backend::CreateShader(std::span<const char> vertexSource, std::span<const char> fragmentSource)
{
    DX12Shader dx12Shader = {
        .VertexShader   = m_shaderCompiler->CompileShader(L"vs_6_5", vertexSource),
        .FragmentShader = m_shaderCompiler->CompileShader(L"ps_6_5", fragmentSource),
    };

    static ui32 shader_handle_workaround = 0;
    auto        shaderHandle             = static_cast<ShaderHandle>(shader_handle_workaround++);

    m_shaders[shaderHandle] = std::move(dx12Shader);

    return shaderHandle;
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
        .Flags       = dxgiSwapChainFlags,
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

void DX12Backend::CreateDescriptorHeaps()
{
    //- Create RTV heap
    D3D12_DESCRIPTOR_HEAP_DESC d3dDescriptorHeapRTV = {
        .Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        .NumDescriptors = k_BackBufferFrames,
        .Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE, // This flag only applies to CBV, SRV, UAV and samplers
        .NodeMask       = 0,
    };
    m_device->CreateDescriptorHeap(&d3dDescriptorHeapRTV, IID_PPV_ARGS(m_descriptorHeapRTV.GetAddressOf()));

    //- Create DSV heap
    D3D12_DESCRIPTOR_HEAP_DESC d3dDescriptorHeapDSV = {
        .Type           = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
        .NumDescriptors = 1,
        .Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        .NodeMask       = 0,
    };
    m_device->CreateDescriptorHeap(&d3dDescriptorHeapDSV, IID_PPV_ARGS(m_descriptorHeapDSV.GetAddressOf()));

    //- Create SRV heap
    D3D12_DESCRIPTOR_HEAP_DESC d3dDescriptorHeapSRV = {
        .Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        .NumDescriptors = k_MaxTextureDescriptors,
        .Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        .NodeMask       = 0,
    };
    m_device->CreateDescriptorHeap(&d3dDescriptorHeapSRV, IID_PPV_ARGS(m_descriptorHeapSRV.GetAddressOf()));

    //- Get Descriptor sizes
    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_srvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void DX12Backend::CreateRenderTargetViews()
{
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

void DX12Backend::CreateDepthStencilView()
{
    //TODO(v.matushkin): <HeapPropertiesUnknown>
    D3D12_HEAP_PROPERTIES d3dHeapProperties = {
        .Type                 = D3D12_HEAP_TYPE_DEFAULT,
        .CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
        .CreationNodeMask     = 1,
        .VisibleNodeMask      = 1,
    };
    DXGI_SAMPLE_DESC dxgiSampleDesc = {
        .Count   = 1,
        .Quality = 0
    };
    // NOTE(v.matushkin): From PIX:
    //  These depth/stencil resources weren't used as shader resources during this capture, but the resources didn't have the
    //  D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE flag set on them at creation time. If the application doesn't use these resources as shader resources,
    //  then consider adding DENY_SHADER_RESOURCE to their creation flags to improve performance on some hardware.
    //  It will be particularly beneficial on AMD GCN 1.2+ hardware that supports "Delta Color Compression" (DCC).
    D3D12_RESOURCE_DESC1 d3dResourceDesc = {
        .Dimension                = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        .Alignment                = 0,
        .Width                    = k_WindowWidth,
        .Height                   = k_WindowHeight,
        .DepthOrArraySize         = 1,
        .MipLevels                = 0,
        .Format                   = k_DepthStencilFormat,
        .SampleDesc               = dxgiSampleDesc,
        .Layout                   = D3D12_TEXTURE_LAYOUT_UNKNOWN,
        .Flags                    = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE,
        // .SamplerFeedbackMipRegion = ,
    };

    D3D12_DEPTH_STENCIL_VALUE d3dDepthStencilValue = {
        .Depth   = k_DepthClearValue,
        .Stencil = 0,
    };
    D3D12_CLEAR_VALUE d3dDepthOptimizedClearValue = {
        .Format       = k_DepthStencilFormat,
        .DepthStencil = d3dDepthStencilValue,
    };

    m_device->CreateCommittedResource2(
        &d3dHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &d3dResourceDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &d3dDepthOptimizedClearValue,
        nullptr,
        IID_PPV_ARGS(m_depthStencil.GetAddressOf())
    );

    // NOTE(v.matushkin): Why it works without setting Texture2D?
    D3D12_DEPTH_STENCIL_VIEW_DESC d3dDepthStencilViewDesc = {
        .Format        = k_DepthStencilFormat,
        .ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
        .Flags         = D3D12_DSV_FLAG_NONE,
        // .Texture2D     = ,
    };
    m_device->CreateDepthStencilView(m_depthStencil.Get(), &d3dDepthStencilViewDesc, m_descriptorHeapDSV->GetCPUDescriptorHandleForHeapStart());
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

    auto hr = m_device->CreateCommittedResource2(
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
        .BaseShaderRegister                = ShaderRegister::tTextureAlbedo,
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

    Microsoft::WRL::ComPtr<ID3DBlob> rootSignature;
    Microsoft::WRL::ComPtr<ID3DBlob> rootSignatureError;
    auto hr = D3D12SerializeVersionedRootSignature(&d3dVersionedRootSignatureDesc, rootSignature.GetAddressOf(), rootSignatureError.GetAddressOf());
    // TODO(v.matushkin): Remove error check from release build?
    if (FAILED(hr))
    {
        OutputDebugStringA((LPSTR)rootSignatureError->GetBufferPointer());
    }

    m_device->CreateRootSignature(0, rootSignature->GetBufferPointer(), rootSignature->GetBufferSize(), IID_PPV_ARGS(m_rootSignature.GetAddressOf()));
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
        .DSVFormat             = k_DepthStencilFormat,
        .SampleDesc            = dxgiSampleDesc,
        .NodeMask              = 0,
        // .CachedPSO             =,
        // .Flags                 =,
    };
    d3dGraphicsPipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    auto hr = m_device->CreateGraphicsPipelineState(&d3dGraphicsPipelineDesc, IID_PPV_ARGS(m_graphicsPipeline.GetAddressOf()));

    // TODO(v.matushkin): <RenderGraph>
    g_IsPipelineInitialized = true;
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
