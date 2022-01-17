#include <Engine/Renderer/Directx11/DX11Backend.hpp>

#include <Engine/EngineSettings.hpp>
#include <Engine/Application/Window.hpp>
#include <Engine/Core/Assert.hpp>
#include <Engine/Renderer/RenderDefaults.hpp>
#include <Engine/Renderer/Directx11/DX11ImGuiRenderContext.hpp>
#include <Engine/Renderer/Directx11/DX11ShaderCompiler.hpp>

#include <d3d11_4.h>

#include <string>


// TODO(v.matushkin):
//  - <RenderGraph>
//    - Connect last RenderPass output to swapchain
//    - <Viewport>
//      Right now I'm setting Viewport to EngineRenderPass output dimensions and I'm lucky that
//      ImGui setting Viewport to the window size when ImGuiRenderPass is rendered.
//      But in the future there should be a public method to set Viewport?
//      Or at least set it in the BeginRenderPass() ?


//- RenderTexture

#define DX11_NO_DEPTH_STENCIL 0
static ui8 dx11_RenderTextureTypeToClearFlags(snv::RenderTextureType renderTextureType)
{
    static const ui8 d3dClearFlags[] = {
        DX11_NO_DEPTH_STENCIL, // NOTE(v.matushkin): Only needed as a padding
        D3D11_CLEAR_DEPTH,
        D3D11_CLEAR_STENCIL,
        D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
    };

    return d3dClearFlags[static_cast<ui8>(renderTextureType)];
}

static DXGI_FORMAT dx11_RenderTextureFormat(snv::RenderTextureFormat renderTextureFormat)
{
    static const DXGI_FORMAT dxgiRenderTextureFormat[] = {
        DXGI_FORMAT_B8G8R8A8_UNORM,
        DXGI_FORMAT_D32_FLOAT,
    };

    return dxgiRenderTextureFormat[static_cast<ui8>(renderTextureFormat)];
}

//- Texture

// NOTE(v.matushkin): What about BGRA/RGBA ?
// NOTE(v.matushkin): Don't know if I should use UINT or UNORM
// TODO(v.matushkin): Apparently there is no support fo 24 bit formats (like RGB8, DEPTH24) on all(almost?) gpus
//  So I guess, I should remove this formats from TextureGraphicsFormat
static DXGI_FORMAT dx11_TextureFormat(snv::TextureFormat textureFormat)
{
    static const DXGI_FORMAT dxgiTextureFormat[] = {
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
        DXGI_FORMAT_D24_UNORM_S8_UINT, // DEPTH32
        DXGI_FORMAT_D32_FLOAT,
    };

    return dxgiTextureFormat[static_cast<ui8>(textureFormat)];
}

static D3D11_TEXTURE_ADDRESS_MODE dx11_TextureWrapMode(snv::TextureWrapMode textureWrapMode)
{
    static const D3D11_TEXTURE_ADDRESS_MODE d3dTextureWrapMode[] = {
        D3D11_TEXTURE_ADDRESS_CLAMP,
        D3D11_TEXTURE_ADDRESS_BORDER,
        D3D11_TEXTURE_ADDRESS_MIRROR_ONCE,
        D3D11_TEXTURE_ADDRESS_MIRROR,
        D3D11_TEXTURE_ADDRESS_WRAP,
    };

    return d3dTextureWrapMode[static_cast<ui8>(textureWrapMode)];
}

//- Shader states

//-- RasterizerState
static D3D11_CULL_MODE dx11_CullMode(snv::CullMode cullMode)
{
    static const D3D11_CULL_MODE d3dCullMode[] = {
        D3D11_CULL_NONE,
        D3D11_CULL_FRONT,
        D3D11_CULL_BACK,
    };

    return d3dCullMode[static_cast<ui8>(cullMode)];
}

static D3D11_FILL_MODE dx11_PolygonMode(snv::PolygonMode polygonMode)
{
    static const D3D11_FILL_MODE d3dPolygonMode[] = {
        D3D11_FILL_SOLID,
        D3D11_FILL_WIREFRAME,
    };

    return d3dPolygonMode[static_cast<ui8>(polygonMode)];
}

static bool dx11_TriangleFrontFace(snv::TriangleFrontFace triangleFrontFace)
{
    return triangleFrontFace == snv::TriangleFrontFace::Clockwise ? false : true;
}

//-- DepthStencilState
static D3D11_COMPARISON_FUNC dx11_CompareFunction(snv::CompareFunction compareFunction)
{
    static const D3D11_COMPARISON_FUNC d3dCompareFunction[] = {
        D3D11_COMPARISON_NEVER,
        D3D11_COMPARISON_LESS,
        D3D11_COMPARISON_EQUAL,
        D3D11_COMPARISON_LESS_EQUAL,
        D3D11_COMPARISON_GREATER,
        D3D11_COMPARISON_NOT_EQUAL,
        D3D11_COMPARISON_GREATER_EQUAL,
        D3D11_COMPARISON_ALWAYS,
    };

    return d3dCompareFunction[static_cast<ui8>(compareFunction)];
}

//-- BlendState
static D3D11_BLEND_OP dx11_BlendOp(snv::BlendOp blendOp)
{
    static const D3D11_BLEND_OP d3dBlendOp[] = {
        D3D11_BLEND_OP_ADD,
        D3D11_BLEND_OP_SUBTRACT,
        D3D11_BLEND_OP_REV_SUBTRACT,
        D3D11_BLEND_OP_MIN,
        D3D11_BLEND_OP_MAX,
    };

    return d3dBlendOp[static_cast<ui8>(blendOp)];
}

static D3D11_BLEND dx11_BlendFactor(snv::BlendFactor blendFactor)
{
    static const D3D11_BLEND d3dBlendFactor[] = {
        D3D11_BLEND_ZERO,
        D3D11_BLEND_ONE,
        D3D11_BLEND_SRC_COLOR,
        D3D11_BLEND_INV_SRC_COLOR,
        D3D11_BLEND_DEST_COLOR,
        D3D11_BLEND_INV_DEST_COLOR,
        D3D11_BLEND_SRC_ALPHA,
        D3D11_BLEND_INV_SRC_ALPHA,
        D3D11_BLEND_DEST_ALPHA,
        D3D11_BLEND_INV_DEST_ALPHA,
        D3D11_BLEND_SRC_ALPHA_SAT,
        D3D11_BLEND_SRC1_COLOR,
        D3D11_BLEND_INV_SRC1_COLOR,
        D3D11_BLEND_SRC1_ALPHA,
        D3D11_BLEND_INV_SRC1_ALPHA,
    };

    return d3dBlendFactor[static_cast<ui8>(blendFactor)];
}

static D3D11_LOGIC_OP dx11_BlendLogicOp(snv::BlendLogicOp blendLogicOp)
{
    static const D3D11_LOGIC_OP d3dBlendLogicOp[] = {
        D3D11_LOGIC_OP_CLEAR,
        D3D11_LOGIC_OP_SET,
        D3D11_LOGIC_OP_COPY,
        D3D11_LOGIC_OP_COPY_INVERTED,
        D3D11_LOGIC_OP_NOOP,
        D3D11_LOGIC_OP_INVERT,
        D3D11_LOGIC_OP_AND,
        D3D11_LOGIC_OP_NAND,
        D3D11_LOGIC_OP_OR,
        D3D11_LOGIC_OP_NOR,
        D3D11_LOGIC_OP_XOR,
        D3D11_LOGIC_OP_EQUIV,
        D3D11_LOGIC_OP_AND_REVERSE,
        D3D11_LOGIC_OP_AND_INVERTED,
        D3D11_LOGIC_OP_OR_REVERSE,
        D3D11_LOGIC_OP_OR_INVERTED,
    };

    return d3dBlendLogicOp[static_cast<ui8>(blendLogicOp)];
}


static ui32 g_BufferHandleWorkaround        = 0;
static ui32 g_RenderPassHandleWorkaround    = 0;
static ui32 g_RenderTextureHandleWorkaround = 0;
static ui32 g_ShaderHandleWorkaround        = 0;
static ui32 g_TextureHandleWorkaround       = 0;


namespace snv
{

DX11Backend::DX11Backend()
{
    CreateDevice();
    CreateSwapchain();

    //- Create ConstantBuffers
    {
        CD3D11_BUFFER_DESC cbPerFrameDesc(sizeof(PerFrame), D3D11_BIND_CONSTANT_BUFFER);
        CD3D11_BUFFER_DESC cbPerDrawDesc(sizeof(PerDraw), D3D11_BIND_CONSTANT_BUFFER);

        m_device->CreateBuffer(&cbPerFrameDesc, nullptr, m_cbPerFrame.GetAddressOf());
        m_device->CreateBuffer(&cbPerDrawDesc, nullptr, m_cbPerDraw.GetAddressOf());
    }

    //- Set default state
    m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    m_deviceContext->GSSetShader(nullptr, nullptr, 0);
    m_deviceContext->HSSetShader(nullptr, nullptr, 0);
    m_deviceContext->DSSetShader(nullptr, nullptr, 0);
    m_deviceContext->CSSetShader(nullptr, nullptr, 0);
}

// NOTE(v.matushkin): Doubt I need this
//DX11Backend::~DX11Backend()
//{
//    for (auto& handleAndBuffer : m_buffers)
//    {
//        auto& buffer = handleAndBuffer.second;
//        buffer.Index.Reset();
//        buffer.Position.Reset();
//        buffer.Normal.Reset();
//        buffer.TexCoord0.Reset();
//    }
//    for (auto& handleAndFramebuffer : m_framebuffers)
//    {
//        auto& framebuffer = handleAndFramebuffer.second;
//        for (auto& colorAttachment : framebuffer.ColorAttachments)
//        {
//            colorAttachment.RTV.Reset();
//            colorAttachment.SRV.Reset(); // NOTE(v.matushkin): Do I need to check if it's not a null?
//            colorAttachment.Texture.Reset();
//        }
//        if (framebuffer.DepthStencilClearFlags != 0)
//        {
//            auto& depthStencilAttachment = framebuffer.DepthStencilAttachment;
//            depthStencilAttachment.DSV.Reset();
//            depthStencilAttachment.SRV.Reset();  // NOTE(v.matushkin): Do I need to check if it's not a null?
//            depthStencilAttachment.Texture.Reset();
//        }
//    }
//    for (auto& handleAndTexture : m_textures)
//    {
//        auto& texture = handleAndTexture.second;
//        texture.SRV.Reset();
//        texture.Sampler.Reset();
//        texture.Texture.Reset();
//    }
//
//#ifdef SNV_GPU_API_DEBUG_ENABLED
//    ComPtr<ID3D11Debug> d3dDebug;
//    m_device.As(&d3dDebug);
//
//    d3dDebug->ReportLiveDeviceObjects(D3D11_RLDO_SUMMARY);
//#endif // SNV_GPU_API_DEBUG_ENABLED
//}


void* DX11Backend::GetNativeRenderTexture(RenderTextureHandle renderTextureHandle)
{
    const auto d3dTextureSrv = m_renderTextures[renderTextureHandle].SRV;
    SNV_ASSERT(d3dTextureSrv != nullptr, "Trying to access nullptr DX11RenderTexture.SRV");
    return reinterpret_cast<void*>(d3dTextureSrv.Get());
}


void DX11Backend::BeginFrame(const glm::mat4x4& localToWorld, const glm::mat4x4& cameraView, const glm::mat4x4& cameraProjection)
{
    //- Update constant buffers
    m_cbPerFrameData._CameraProjection = cameraProjection;
    m_cbPerFrameData._CameraView       = cameraView;
    m_cbPerDrawData._ObjectToWorld     = localToWorld;
    // TODO(v.matushkin): UpdateSubresource1 ?
    //  And learn what this parameters do
    m_deviceContext->UpdateSubresource(m_cbPerFrame.Get(), 0, nullptr, &m_cbPerFrameData, 0, 0);
    m_deviceContext->UpdateSubresource(m_cbPerDraw.Get(), 0, nullptr, &m_cbPerDrawData, 0, 0);

    //- Set Vertex camera/model buffers
    {
        ID3D11Buffer* constantBuffers[]{m_cbPerFrame.Get(), m_cbPerDraw.Get()};
        m_deviceContext->VSSetConstantBuffers(0, 2, constantBuffers); // NOTE(v.matushkin): VSSetConstantBuffers1 ?
    }
}

void DX11Backend::BeginRenderPass(RenderPassHandle renderPassHandle)
{
    const auto& dx11RenderPass = m_renderPasses[renderPassHandle];

    const auto& dx11DepthStencilRenderTexture = dx11RenderPass.DepthStencilAttachment;
    const auto  d3dDepthStencilClearFlags     = dx11RenderPass.DepthStencilClearFlags;

    auto* d3dDepthStencilView = d3dDepthStencilClearFlags != DX11_NO_DEPTH_STENCIL
                              ? dx11DepthStencilRenderTexture.DSV.Get()
                              : nullptr;

    //- Set RenderTargets
    m_deviceContext->OMSetRenderTargets(dx11RenderPass.ColorRTVs.size(), dx11RenderPass.ColorRTVs.data(), d3dDepthStencilView);

    //- Clear Color attachments
    for (const auto& colorAttachment : dx11RenderPass.ColorAttachments)
    {
        if (colorAttachment.LoadAction == RenderTextureLoadAction::Clear)
        {
            m_deviceContext->ClearRenderTargetView(colorAttachment.RTV.Get(), colorAttachment.ClearValue.Color);
        }
    }
    //- Clear DepthStencil attachment
    if (d3dDepthStencilView != nullptr && dx11DepthStencilRenderTexture.LoadAction == RenderTextureLoadAction::Clear)
    {
        const auto depthStencilClearValue = dx11DepthStencilRenderTexture.ClearValue.DepthStencil;
        m_deviceContext->ClearDepthStencilView(
            d3dDepthStencilView,
            d3dDepthStencilClearFlags,
            depthStencilClearValue.Depth,
            depthStencilClearValue.Stencil
        );
    }
}

void DX11Backend::BeginRenderPass(RenderPassHandle renderPassHandle, RenderTextureHandle input)
{
    BeginRenderPass(renderPassHandle);
}

void DX11Backend::EndFrame()
{
    // TODO(v.matushkin): VSync settings
    m_swapChain->Present(0, 0);
}


void DX11Backend::BindShader(ShaderHandle shaderHandle)
{
    const auto& dx11Shader = m_shaders[shaderHandle];

    m_deviceContext->IASetInputLayout(dx11Shader.InputLayout.Get());
    m_deviceContext->VSSetShader(dx11Shader.VertexShader.Get(), nullptr, 0);
    m_deviceContext->RSSetState(dx11Shader.RasterizerState.Get());
    m_deviceContext->PSSetShader(dx11Shader.PixelShader.Get(), nullptr, 0);
    // TODO(v.matushkin): StencilRef ?
    m_deviceContext->OMSetDepthStencilState(dx11Shader.DepthStencilState.Get(), D3D11_DEFAULT_STENCIL_REFERENCE);
    // TODO(v.matushkin): BlendFactor, SampleMask ? BlendFactor = nullptr -> {1,1,1,1}
    m_deviceContext->OMSetBlendState(dx11Shader.BlendState.Get(), nullptr, D3D11_DEFAULT_SAMPLE_MASK);
}


void DX11Backend::DrawBuffer(TextureHandle textureHandle, BufferHandle bufferHandle, i32 indexCount, i32 vertexCount)
{
    //- Set Index/Vertex buffers
    // TODO(v.matushkin): Rename, there is no GraphicsBuffer anymore
    const auto& buffer = m_buffers[bufferHandle];

    ID3D11Buffer* d3dBuffers[] = {buffer.Position.Get(), buffer.Normal.Get(), buffer.TexCoord0.Get()};
    ui32          strides[]    = {sizeof(f32) * 3, sizeof(f32) * 3, sizeof(f32) * 3};
    ui32          offset[]     = {0, 0, 0};

    m_deviceContext->IASetVertexBuffers(0, 3, d3dBuffers, strides, offset);
    m_deviceContext->IASetIndexBuffer(buffer.Index.Get(), DXGI_FORMAT_R32_UINT, 0);

    //- Set Material Texture
    const auto& texture = m_textures[textureHandle];
    m_deviceContext->PSSetShaderResources(0, 1, texture.SRV.GetAddressOf());
    m_deviceContext->PSSetSamplers(0, 1, texture.Sampler.GetAddressOf());

    m_deviceContext->DrawIndexed(indexCount, 0, 0);
}


IImGuiRenderContext* DX11Backend::CreateImGuiRenderContext()
{
    return new DX11ImGuiRenderContext(this, m_device.Get(), m_deviceContext.Get(), m_factory.Get());
}


RenderTextureHandle DX11Backend::CreateRenderTexture(const RenderTextureDesc& renderTextureDesc)
{
    const auto dxgiRenderTextureFormat = dx11_RenderTextureFormat(renderTextureDesc.Format);
    const auto renderTextureType       = renderTextureDesc.RenderTextureType();
    const auto isColorRenderTexture    = renderTextureType == RenderTextureType::Color;
    const auto isUsageShaderRead       = renderTextureDesc.Usage == RenderTextureUsage::ShaderRead;

    //- Create RenderTexture
    ID3D11Texture2D1* d3dRenderTexture;
    {
        ui32 d3dTextureBindFlags = isColorRenderTexture ? D3D11_BIND_RENDER_TARGET : D3D11_BIND_DEPTH_STENCIL;
        if (isUsageShaderRead)
        {
            d3dTextureBindFlags |= D3D11_BIND_SHADER_RESOURCE;
        }
        D3D11_TEXTURE2D_DESC1 d3dDepthStencilDesc = {
            .Width          = renderTextureDesc.Width,
            .Height         = renderTextureDesc.Height,
            .MipLevels      = 1,
            .ArraySize      = 1,
            .Format         = dxgiRenderTextureFormat,
            .SampleDesc     = {.Count = 1, .Quality = 0},
            .Usage          = D3D11_USAGE_DEFAULT,
            .BindFlags      = d3dTextureBindFlags,
            .CPUAccessFlags = 0,
            .MiscFlags      = 0,                              // NOTE(v.matushkin): Whats this?
            .TextureLayout  = D3D11_TEXTURE_LAYOUT_UNDEFINED, // Can use only UNDEFINED if CPUAccessFlags = 0
        };
        m_device->CreateTexture2D1(&d3dDepthStencilDesc, nullptr, &d3dRenderTexture);
    }

    //- Create RTV or DSV
    ID3D11RenderTargetView* d3dRenderTextureRtv;
    ID3D11DepthStencilView* d3dRenderTextureDsv;

    if (isColorRenderTexture)
    {
        d3dRenderTextureDsv = nullptr;

        // NOTE(v.matushkin): There is a D3D11_RENDER_TARGET_VIEW_DESC1, but it seems useless
        D3D11_RENDER_TARGET_VIEW_DESC d3dRtvDesc = {
            .Format        = dxgiRenderTextureFormat,
            .ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D,
            .Texture2D     = {.MipSlice = 0},
        };
        m_device->CreateRenderTargetView(d3dRenderTexture, &d3dRtvDesc, &d3dRenderTextureRtv);
    }
    else
    {
        d3dRenderTextureRtv = nullptr;

        D3D11_DEPTH_STENCIL_VIEW_DESC d3dDsvDesc = {
            .Format        = dxgiRenderTextureFormat,
            .ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D,
            .Flags         = 0, // NOTE(v.matushkin): D3D11_DSV_READ_ONLY_* ?
            .Texture2D     = {.MipSlice = 0},
        };
        m_device->CreateDepthStencilView(d3dRenderTexture, &d3dDsvDesc, &d3dRenderTextureDsv);
    }

    //- Create SRV
    ID3D11ShaderResourceView* d3dRenderTextureSrv;

    if (isUsageShaderRead)
    {
        D3D11_TEX2D_SRV d3dSrvTex2D = {
            .MostDetailedMip = 0,
            .MipLevels       = 1,
        };
        D3D11_SHADER_RESOURCE_VIEW_DESC d3dSrvDesc = {
            .Format        = dxgiRenderTextureFormat,
            .ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
            .Texture2D     = d3dSrvTex2D,
        };
        m_device->CreateShaderResourceView(d3dRenderTexture, &d3dSrvDesc, &d3dRenderTextureSrv);
    }
    else
    {
        d3dRenderTextureSrv = nullptr;
    }

    const auto renderTextureHandle = static_cast<RenderTextureHandle>(g_RenderTextureHandleWorkaround++);
    m_renderTextures[renderTextureHandle] = {
        .Texture    = d3dRenderTexture,
        .RTV        = d3dRenderTextureRtv,
        .DSV        = d3dRenderTextureDsv,
        .SRV        = d3dRenderTextureSrv,
        .ClearValue = renderTextureDesc.ClearValue,
        .LoadAction = renderTextureDesc.LoadAction,
        .Type       = renderTextureType,
    };

    return renderTextureHandle;
}

RenderPassHandle DX11Backend::CreateRenderPass(const RenderPassDesc& renderPassDesc)
{
    DX11RenderPass dx11RenderPass;

    for (const auto colorAttachmentHandle : renderPassDesc.ColorAttachments)
    {
        const auto& dx11RenderTexture = m_renderTextures[colorAttachmentHandle];
        dx11RenderPass.ColorAttachments.push_back(dx11RenderTexture);
        dx11RenderPass.ColorRTVs.push_back(dx11RenderTexture.RTV.Get());
    }

    if (renderPassDesc.DepthStencilAttachment.has_value())
    {
        const auto& dx11RenderTexture = m_renderTextures[renderPassDesc.DepthStencilAttachment.value()];
        dx11RenderPass.DepthStencilAttachment = dx11RenderTexture;
        dx11RenderPass.DepthStencilClearFlags = dx11_RenderTextureTypeToClearFlags(dx11RenderTexture.Type);
    }
    else
    {
        dx11RenderPass.DepthStencilClearFlags = DX11_NO_DEPTH_STENCIL;
    }

    const auto renderPassHandle      = static_cast<RenderPassHandle>(g_RenderPassHandleWorkaround++);
    m_renderPasses[renderPassHandle] = std::move(dx11RenderPass);

    return renderPassHandle;
}


BufferHandle DX11Backend::CreateBuffer(
    std::span<const std::byte>              indexData,
    std::span<const std::byte>              vertexData,
    const std::vector<VertexAttributeDesc>& vertexLayout
)
{
    DX11Buffer dx11Buffer;

    CD3D11_BUFFER_DESC     d3dIndexBufferDesc(indexData.size_bytes(), D3D11_BIND_INDEX_BUFFER);
    D3D11_SUBRESOURCE_DATA d3dIndexSubresourceData = {
        .pSysMem          = indexData.data(),
        .SysMemPitch      = 0,
        .SysMemSlicePitch = 0,
    };
    m_device->CreateBuffer(&d3dIndexBufferDesc, &d3dIndexSubresourceData, dx11Buffer.Index.GetAddressOf());

    ID3D11Buffer** d3dBuffers[] = {
        dx11Buffer.Position.GetAddressOf(),
        dx11Buffer.Normal.GetAddressOf(),
        dx11Buffer.TexCoord0.GetAddressOf(),
    };

    for (ui32 i = 0; i < vertexLayout.size(); ++i)
    {
        // TODO(v.matushkin): Adjust VertexAttributeDescriptor to remove this hacks
        const auto& vertexAttribute = vertexLayout[i];
        const auto  currOffset      = vertexAttribute.Offset;
        const auto  nextOffset      = (i + 1) < vertexLayout.size() ? vertexLayout[i + 1].Offset : vertexData.size_bytes();
        const auto  attributeSize   = (nextOffset - currOffset);

        CD3D11_BUFFER_DESC     d3dAttributeBufferDesc(attributeSize, D3D11_BIND_VERTEX_BUFFER);
        D3D11_SUBRESOURCE_DATA d3dAttributeSubresourceData = {
            .pSysMem          = vertexData.data() + currOffset, // TODO(v.matushkin): Will this work?
            .SysMemPitch      = 0,
            .SysMemSlicePitch = 0,
        };
        // TODO(v.matushkin): Create mupltiple buffers in one call?
        m_device->CreateBuffer(&d3dAttributeBufferDesc, &d3dAttributeSubresourceData, d3dBuffers[i]);
    }

    const auto graphicsBufferHandle = static_cast<BufferHandle>(g_BufferHandleWorkaround++);
    m_buffers[graphicsBufferHandle] = dx11Buffer;

    return graphicsBufferHandle;
}

TextureHandle DX11Backend::CreateTexture(const TextureDesc& textureDesc, const ui8* textureData)
{
    const auto dxgiTextureFormat = dx11_TextureFormat(textureDesc.Format);

    DX11Texture dx11Texture;
    //- Create Texture
    {
        DXGI_SAMPLE_DESC      dxgiSampleDesc = {.Count = 1, .Quality = 0};
        D3D11_TEXTURE2D_DESC1 d3dTextureDesc = {
            .Width          = textureDesc.Width,
            .Height         = textureDesc.Height,
            .MipLevels      = 1, // TODO(v.matushkin): No way to use texture without mip, 0 = generate a full set of subtextures?
            .ArraySize      = 1,
            .Format         = dxgiTextureFormat,
            .SampleDesc     = dxgiSampleDesc,
            .Usage          = D3D11_USAGE_DEFAULT,
            .BindFlags      = D3D11_BIND_SHADER_RESOURCE,
            .CPUAccessFlags = 0,
            .MiscFlags      = 0,                             // NOTE(v.matushkin): Whats this?
            .TextureLayout  = D3D11_TEXTURE_LAYOUT_UNDEFINED // Can use only UNDEFINED if CPUAccessFlags = 0
        };
        D3D11_SUBRESOURCE_DATA d3dSubresourceData = {
            .pSysMem          = textureData,
            .SysMemPitch      = textureDesc.Width * 4, // TODO(v.matushkin): Remove hardcoded shit
            .SysMemSlicePitch = 0,
        };
        m_device->CreateTexture2D1(&d3dTextureDesc, &d3dSubresourceData, dx11Texture.Texture.GetAddressOf());
    }
    //- Create Texture SRV
    {
        D3D11_TEX2D_SRV d3dSrvTex2D = {
            .MostDetailedMip = 0,
            .MipLevels       = 1,
        };
        // TODO(v.matushkin): Don't know how to use ID3D11ShaderResourceView1
        //  if m_deviceContext->PSSetShaderResources takes only ID3D11ShaderResourceView
        D3D11_SHADER_RESOURCE_VIEW_DESC d3dSrvDesc = {
            .Format        = dxgiTextureFormat,
            .ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
            .Texture2D     = d3dSrvTex2D,
        };
        m_device->CreateShaderResourceView(dx11Texture.Texture.Get(), &d3dSrvDesc, dx11Texture.SRV.GetAddressOf());
    }
    //- Create Texture Sampler
    {
        const auto d3dTextureWrapMode = dx11_TextureWrapMode(textureDesc.WrapMode);

        D3D11_SAMPLER_DESC d3dSamplerDesc = {
            .Filter         = D3D11_FILTER_MIN_MAG_MIP_POINT,
            .AddressU       = d3dTextureWrapMode,
            .AddressV       = d3dTextureWrapMode,
            .AddressW       = D3D11_TEXTURE_ADDRESS_CLAMP, // NOTE(v.matushkin): How to handle this for 2D textures?
            .MipLODBias     = 0,
            .MaxAnisotropy  = 0, // NOTE(v.matushkin): Only used for ANISOTROPIC filter?
            .ComparisonFunc = D3D11_COMPARISON_NEVER,
            // .BorderColor      // NOTE(v.matushkin): Only used if D3D11_TEXTURE_ADDRESS_BORDER is specified in Address*
            .MinLOD         = 0,
            .MaxLOD         = D3D11_FLOAT32_MAX,
        };
        m_device->CreateSamplerState(&d3dSamplerDesc, dx11Texture.Sampler.GetAddressOf());
    }

    const auto textureHandle = static_cast<TextureHandle>(g_TextureHandleWorkaround++);
    m_textures[textureHandle] = dx11Texture;

    return textureHandle;
}

ShaderHandle DX11Backend::CreateShader(const ShaderDesc& shaderDesc)
{
    // https://github-wiki-see.page/m/Microsoft/DirectXTK/wiki/CommonStates

    //- Create RasterizerState
    ID3D11RasterizerState2* d3dRasterizerState;
    {
        const auto rasterizerStateDesc = shaderDesc.RasterizerStateDesc;
        const auto isImGuiShader       = shaderDesc.Name == RenderDefaults::ImGuiShaderName;

        D3D11_RASTERIZER_DESC2 d3dRasterizerStateDesc = {
            .FillMode              = dx11_PolygonMode(rasterizerStateDesc.PolygonMode),
            .CullMode              = dx11_CullMode(rasterizerStateDesc.CullMode),
            .FrontCounterClockwise = !isImGuiShader,
            .DepthBias             = D3D11_DEFAULT_DEPTH_BIAS,
            .DepthBiasClamp        = D3D11_DEFAULT_DEPTH_BIAS_CLAMP,
            .SlopeScaledDepthBias  = D3D11_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
            .DepthClipEnable       = true,
            .ScissorEnable         = isImGuiShader,
            .MultisampleEnable     = false,
            .AntialiasedLineEnable = false,
            .ForcedSampleCount     = 0,
            .ConservativeRaster    = D3D11_CONSERVATIVE_RASTERIZATION_MODE_OFF,
        };
        m_device->CreateRasterizerState2(&d3dRasterizerStateDesc, &d3dRasterizerState);

        if (d3dRasterizerState == nullptr)
        {
            LOG_ERROR("Failed to create ID3D11RasterizerState2 for '{}' shader", shaderDesc.Name);
        }
    }
    //- Create DepthStencilState
    ID3D11DepthStencilState* d3dDepthStencilState;
    {
        const auto depthStencilStateDesc = shaderDesc.DepthStencilStateDesc;

        D3D11_DEPTH_STENCILOP_DESC d3dDepthStencilOpDesc = {
            .StencilFailOp      = D3D11_STENCIL_OP_KEEP,
            .StencilDepthFailOp = D3D11_STENCIL_OP_KEEP,
            .StencilPassOp      = D3D11_STENCIL_OP_KEEP,
            .StencilFunc        = D3D11_COMPARISON_ALWAYS,
        };
        D3D11_DEPTH_STENCIL_DESC d3dDepthStencilStateDesc = {
            .DepthEnable      = depthStencilStateDesc.DepthTestEnable,
            .DepthWriteMask   = depthStencilStateDesc.DepthWriteEnable ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO,
            .DepthFunc        = dx11_CompareFunction(depthStencilStateDesc.DepthCompareFunction),
            .StencilEnable    = false,
            .StencilReadMask  = 0,                          // Default is 'D3D11_DEFAULT_STENCIL_READ_MASK'
            .StencilWriteMask = 0,                          // Default is 'D3D11_DEFAULT_STENCIL_WRITE_MASK'
            .FrontFace        = d3dDepthStencilOpDesc,
            .BackFace         = d3dDepthStencilOpDesc,
        };
        m_device->CreateDepthStencilState(&d3dDepthStencilStateDesc, &d3dDepthStencilState);

        if (d3dDepthStencilState == nullptr)
        {
            LOG_ERROR("Failed to create ID3D11DepthStencilState for '{}' shader", shaderDesc.Name);
        }
    }
    //- Create BlendState
    ID3D11BlendState1* d3dBlendState;
    {
        const auto blendStateDesc = shaderDesc.BlendStateDesc;

        D3D11_RENDER_TARGET_BLEND_DESC1 d3dRenderTargetBlendDesc = {
            // NOTE(v.matushkin): May be there is no need to set this if BlendMode::Off ?
            .RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
        };

        if (blendStateDesc.BlendMode == BlendMode::BlendOp)
        {
            d3dRenderTargetBlendDesc.BlendEnable    = true;
            d3dRenderTargetBlendDesc.SrcBlend       = dx11_BlendFactor(blendStateDesc.ColorSrcBlendFactor);
            d3dRenderTargetBlendDesc.DestBlend      = dx11_BlendFactor(blendStateDesc.ColorDstBlendFactor);
            d3dRenderTargetBlendDesc.BlendOp        = dx11_BlendOp(blendStateDesc.ColorBlendOp);
            d3dRenderTargetBlendDesc.SrcBlendAlpha  = dx11_BlendFactor(blendStateDesc.AlphaSrcBlendFactor);
            d3dRenderTargetBlendDesc.DestBlendAlpha = dx11_BlendFactor(blendStateDesc.AlphaDstBlendFactor);
            d3dRenderTargetBlendDesc.BlendOpAlpha   = dx11_BlendOp(blendStateDesc.AlphaBlendOp);
        }
        else if (blendStateDesc.BlendMode == BlendMode::LogicOp)
        {
            d3dRenderTargetBlendDesc.LogicOpEnable = true;
            d3dRenderTargetBlendDesc.LogicOp       = dx11_BlendLogicOp(blendStateDesc.LogicOp);
        }

        D3D11_BLEND_DESC1 d3dBlendStateDesc;
        d3dBlendStateDesc.AlphaToCoverageEnable  = false;
        d3dBlendStateDesc.IndependentBlendEnable = RenderDefaults::IndependentBlendEnable;
        d3dBlendStateDesc.RenderTarget[0]        = d3dRenderTargetBlendDesc;

        m_device->CreateBlendState1(&d3dBlendStateDesc, &d3dBlendState);

        if (d3dBlendState == nullptr)
        {
            LOG_ERROR("Failed to create ID3D11BlendState1 for '{}' shader", shaderDesc.Name);
        }
    }

    //- Create InputLayout and shaders
    ID3D11InputLayout*  d3dInputLayout;
    ID3D11VertexShader* d3dVertexShader;
    ID3D11PixelShader*  d3dPixelShader;
    {
        // NOTE(v.matushkin): How to handle shader compilation errors?
        const auto dx11ShaderCompiler = DX11ShaderCompiler(shaderDesc);

        const auto d3dInputLayoutDesc = dx11ShaderCompiler.GetInputLayoutDesc();
        const auto vertexShaderBuffer = dx11ShaderCompiler.GetVertexShaderBuffer();
        const auto pixelShaderBuffer  = dx11ShaderCompiler.GetPixelShaderBuffer();

        const auto d3dCreateInputResult = m_device->CreateInputLayout(
            d3dInputLayoutDesc.data(),
            d3dInputLayoutDesc.size(),
            vertexShaderBuffer.Ptr,
            vertexShaderBuffer.Size,
            &d3dInputLayout
        );
        if (FAILED(d3dCreateInputResult))
        {
            LOG_ERROR("Failed to create ID3D11InputLayout for '{}' shader", shaderDesc.Name);
        }

        m_device->CreateVertexShader(vertexShaderBuffer.Ptr, vertexShaderBuffer.Size, nullptr, &d3dVertexShader);
        m_device->CreatePixelShader(pixelShaderBuffer.Ptr, pixelShaderBuffer.Size, nullptr, &d3dPixelShader);
    }

    const auto shaderHandle = static_cast<ShaderHandle>(g_ShaderHandleWorkaround++);
    m_shaders[shaderHandle] = {
        .InputLayout       = d3dInputLayout,
        .VertexShader      = d3dVertexShader,
        .RasterizerState   = d3dRasterizerState,
        .PixelShader       = d3dPixelShader,
        .DepthStencilState = d3dDepthStencilState,
        .BlendState        = d3dBlendState,
    };

    return shaderHandle;
}


void DX11Backend::CreateDevice()
{
    //- Create Factory and Adapter
    ComPtr<IDXGIAdapter3> dxgiAdapter3;
    {
        ui32 dxgiFactoryFlags = 0;
#ifdef SNV_GPU_API_DEBUG_ENABLED
        dxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG; // NOTE(v.matushkin): Do I even need to set this?
#endif // SNV_GPU_API_DEBUG_ENABLED

        ComPtr<IDXGIAdapter1> dxgiAdapter;

        CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(m_factory.GetAddressOf()));
        m_factory->EnumAdapters1(0, dxgiAdapter.GetAddressOf());
        dxgiAdapter.As(&dxgiAdapter3);

        //-- Get GPU description
        DXGI_ADAPTER_DESC2 dxgiAdapterDesc;
        dxgiAdapter3->GetDesc2(&dxgiAdapterDesc);
        // TODO: Fix this shit with spdlog wide string option support? (need custom conan package then)
        std::wstring wstr(dxgiAdapterDesc.Description);
        std::string  desc(wstr.begin(), wstr.end());
        LOG_INFO("DirectX 11\nAdapter description: {}", desc);
    }

    //- Create Device and DeviceContext
    D3D_FEATURE_LEVEL d3dFeatureLevels[] = {D3D_FEATURE_LEVEL_11_1};

    // NOTE(v.matushkin): Do I even need this?
    //  This flag adds support for surfaces with a color-channel ordering different
    //  from the API default. It is required for compatibility with Direct2D.
    ui32 d3dDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef SNV_GPU_API_DEBUG_ENABLED
    d3dDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif // SNV_GPU_API_DEBUG_ENABLED

    ComPtr<ID3D11Device>        d3dDevice;
    ComPtr<ID3D11DeviceContext> d3dDeviceContext;

    D3D_FEATURE_LEVEL useless_feature_level_variable;

    D3D11CreateDevice(
        dxgiAdapter3.Get(),
        D3D_DRIVER_TYPE_UNKNOWN, // UNKNOWN if pAdapter is not null
        0,                       // Should be 0 unless the driver is D3D_DRIVER_TYPE_SOFTWARE.
        d3dDeviceFlags,
        d3dFeatureLevels,
        ARRAYSIZE(d3dFeatureLevels),
        D3D11_SDK_VERSION,        // Always set this to D3D11_SDK_VERSION for Windows Store apps. Do I need to?
        d3dDevice.GetAddressOf(),
        &useless_feature_level_variable,
        d3dDeviceContext.GetAddressOf()
    );

    d3dDevice.As(&m_device);
    d3dDeviceContext.As(&m_deviceContext);
}

void DX11Backend::CreateSwapchain()
{
    const auto& windowSettings   = ApplicationSettings::WindowSettings;
    const auto& graphicsSettings = EngineSettings::GraphicsSettings;

    //- Create Swapchain
    DXGI_SAMPLE_DESC dxgiSwapChainSampleDesc = {
        .Count   = 1, // multisampling setting
        .Quality = 0  // vendor-specific flag
    };
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
        // TODO(v.matushkin): <SwapchainCreation/Format>
        .Format      = dx11_RenderTextureFormat(graphicsSettings.SwapchainFormat),
        .Stereo      = false,
        .SampleDesc  = dxgiSwapChainSampleDesc,
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = 2, // TODO(v.matushkin): Hardcoded
        .Scaling     = DXGI_SCALING_STRETCH,                // TODO(v.matushkin): Play with this
        .SwapEffect  = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL,
        .AlphaMode   = DXGI_ALPHA_MODE_IGNORE,              // NOTE(v.matushkin): Don't know
        // .Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH  // TODO(v.matushkin): I guess at least I should use this flag?
    };

    ComPtr<IDXGISwapChain1> dxgiSwapChain;

    m_factory->CreateSwapChainForHwnd(
        m_device.Get(),
        Window::GetWin32Window(),
        &dxgiSwapChainDesc,
        nullptr, // TODO(v.matushkin): Fullscreen swap chain?
        nullptr, // NOTE(v.matushkin): Useless?
        dxgiSwapChain.GetAddressOf()
    );
    dxgiSwapChain.As(&m_swapChain);

    //- Create Swapchain Framebuffer
    DX11RenderTexture colorAttachment = {
        .DSV        = nullptr,
        .SRV        = nullptr,
        .ClearValue = {.Color = {0.0f, 0.0f, 0.0f, 0.0f}},
        .LoadAction = RenderTextureLoadAction::Clear
    };
    m_swapChain->GetBuffer(0, IID_PPV_ARGS(colorAttachment.Texture.GetAddressOf()));
    m_device->CreateRenderTargetView(colorAttachment.Texture.Get(), nullptr, colorAttachment.RTV.GetAddressOf());

    m_swapchainRenderPassHandle                 = static_cast<RenderPassHandle>(g_RenderPassHandleWorkaround++);
    m_renderPasses[m_swapchainRenderPassHandle] = {
        .ColorAttachments       = {colorAttachment},
        .ColorRTVs              = {colorAttachment.RTV.Get()},
        // .DepthStencilAttachment
        .DepthStencilClearFlags = DX11_NO_DEPTH_STENCIL,
    };

    //- Setup Viewport
    // TODO(v.matushkin): <RenderGraph/Viewport>
    D3D11_VIEWPORT d3dViewport = {
        .Width    = static_cast<f32>(graphicsSettings.RenderWidth),
        .Height   = static_cast<f32>(graphicsSettings.RenderHeight),
        .MinDepth = 0,
        .MaxDepth = 1,
    };
    m_deviceContext->RSSetViewports(1, &d3dViewport);
}

} // namespace snv


#undef DX11_NO_DEPTH_STENCIL
