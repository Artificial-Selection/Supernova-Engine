#include <Engine/Renderer/Directx11/DX11Backend.hpp>
#include <Engine/Renderer/Directx11/DX11ImGuiRenderContext.hpp>

#include <Engine/EngineSettings.hpp>
#include <Engine/Application/Window.hpp>
#include <Engine/Core/Assert.hpp>

#include <d3d11_4.h>
#include <d3dcompiler.h>

#include <string>


// TODO(v.matushkin):
//  - <RenderGraph>
//    - Connect last RenderPass output to swapchain
//    - <Viewport>
//      Right now I'm setting Viewport to EngineRenderPass output dimensions and I'm lucky that
//      ImGui setting Viewport to the window size when ImGuiRenderPass is rendered.
//      But in the future there should be a public method to set Viewport?
//      Or at least set it in the BeginRenderPass() ?


// TODO(v.matushkin): Find out how D3D11_APPEND_ALIGNED_ELEMENT works
// TODO(v.matushkin): TEXCOORD to DXGI_FORMAT_R32G32_FLOAT
const D3D11_INPUT_ELEMENT_DESC k_InputElementDesc[] = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 2, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
};

// NOTE(v.matushkin): Questionable naming
const ui32 dx11_DepthStencilType[] = {
    0,
    D3D11_CLEAR_DEPTH,
    D3D11_CLEAR_STENCIL,
    D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
};

const DXGI_FORMAT dx11_RenderTextureFormat[] = {
    DXGI_FORMAT_B8G8R8A8_UNORM,
    DXGI_FORMAT_D32_FLOAT,
};

// NOTE(v.matushkin): What about BGRA/RGBA ?
// NOTE(v.matushkin): Don't know if I should use UINT or UNORM
// TODO(v.matushkin): Apparently there is no support fo 24 bit formats (like RGB8, DEPTH24) on all(almost?) gpus
//  So I guess, I should remove this formats from TextureGraphicsFormat
const DXGI_FORMAT dx11_TextureFormat[] = {
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

const D3D11_TEXTURE_ADDRESS_MODE dx11_TextureWrapMode[] = {
    D3D11_TEXTURE_ADDRESS_CLAMP,
    D3D11_TEXTURE_ADDRESS_BORDER,
    D3D11_TEXTURE_ADDRESS_MIRROR_ONCE,
    D3D11_TEXTURE_ADDRESS_MIRROR,
    D3D11_TEXTURE_ADDRESS_WRAP,
};

const DXGI_FORMAT k_SwapchainFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

static ui32 g_BufferHandleWorkaround        = 0;
static ui32 g_FramebufferHandleWorkaround   = 0;
static ui32 g_RenderTextureHandleWorkaround = 0;
static ui32 g_ShaderHandleWorkaround        = 0;
static ui32 g_TextureHandleWorkaround       = 0;


namespace snv
{

DX11Backend::DX11Backend()
{
    CreateDevice();
    CreateSwapchain();

    //- Setup Rasterizer
    {
        // NOTE(v.matuskin): Changed FrontCounterClockwise to true, everything else is default values
        //  FrontCounterClockwise is false by default, and in OpenGL it's true by default
        D3D11_RASTERIZER_DESC2 d3dRasterizerDesc = {
            .FillMode              = D3D11_FILL_SOLID,
            .CullMode              = D3D11_CULL_BACK,
            .FrontCounterClockwise = true,
            .DepthBias             = D3D11_DEFAULT_DEPTH_BIAS,
            .DepthBiasClamp        = D3D11_DEFAULT_DEPTH_BIAS_CLAMP,
            .SlopeScaledDepthBias  = D3D11_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
            .DepthClipEnable       = true,
            .ScissorEnable         = false,
            .MultisampleEnable     = false,
            .AntialiasedLineEnable = false,
            .ForcedSampleCount     = 0,
            .ConservativeRaster    = D3D11_CONSERVATIVE_RASTERIZATION_MODE_OFF,
        };
        ID3D11RasterizerState2* d3dRasterizerState;
        m_device->CreateRasterizerState2(&d3dRasterizerDesc, &d3dRasterizerState);
        m_deviceContext->RSSetState(d3dRasterizerState);
    }
    //- Create ConstantBuffers
    {
        CD3D11_BUFFER_DESC cbPerFrameDesc(sizeof(PerFrame), D3D11_BIND_CONSTANT_BUFFER);
        CD3D11_BUFFER_DESC cbPerDrawDesc(sizeof(PerDraw), D3D11_BIND_CONSTANT_BUFFER);

        m_device->CreateBuffer(&cbPerFrameDesc, nullptr, m_cbPerFrame.GetAddressOf());
        m_device->CreateBuffer(&cbPerDrawDesc, nullptr, m_cbPerDraw.GetAddressOf());
    }
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


void DX11Backend::EnableBlend()
{
}

void DX11Backend::EnableDepthTest()
{
    // NOTE(v.matushkin): Use ID3D11DepthStencilState?
    // https://github-wiki-see.page/m/Microsoft/DirectXTK/wiki/CommonStates
    //  DepthNone
    //  CD3D11_DEPTH_STENCIL_DESC desc(def);
    //  desc.DepthEnable    = FALSE;
    //  desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    //  desc.DepthFunc      = D3D11_COMPARISON_LESS_EQUAL;
    //
    //  // DepthDefault
    //  CD3D11_DEPTH_STENCIL_DESC desc(def);
    //  desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    //
    //  // DepthRead
    //  CD3D11_DEPTH_STENCIL_DESC desc(def);
    //  desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    //  desc.DepthFunc      = D3D11_COMPARISON_LESS_EQUAL;
}


void* DX11Backend::GetNativeRenderTexture(RenderTextureHandle renderTextureHandle)
{
    const auto d3dTextureSrv = m_renderTextures[renderTextureHandle].SRV;
    SNV_ASSERT(d3dTextureSrv != nullptr, "Trying to access nullptr DX11RenderTexture.SRV");
    return reinterpret_cast<void*>(d3dTextureSrv.Get());
}


void DX11Backend::SetBlendFunction(BlendFactor source, BlendFactor destination)
{
}

void DX11Backend::SetClearColor(f32 r, f32 g, f32 b, f32 a)
{
    m_clearColor[0] = r;
    m_clearColor[1] = g;
    m_clearColor[2] = b;
    m_clearColor[3] = a;
}

void DX11Backend::SetDepthFunction(DepthFunction depthFunction)
{}

void DX11Backend::SetViewport(i32 x, i32 y, i32 width, i32 height)
{}


void DX11Backend::Clear(BufferBit bufferBitMask)
{}


void DX11Backend::BeginFrame(const glm::mat4x4& localToWorld, const glm::mat4x4& cameraView, const glm::mat4x4& cameraProjection)
{
    // TODO(v.matushkin): Shouldn't get shader like this, tmp workaround
    const auto& shader = m_shaders.begin()->second;

    //- Update constant buffers
    m_cbPerFrameData._CameraProjection = cameraProjection;
    m_cbPerFrameData._CameraView       = cameraView;
    m_cbPerDrawData._ObjectToWorld     = localToWorld;
    // TODO(v.matushkin): UpdateSubresource1 ?
    //  And learn what this parameters do
    m_deviceContext->UpdateSubresource(m_cbPerFrame.Get(), 0, nullptr, &m_cbPerFrameData, 0, 0);
    m_deviceContext->UpdateSubresource(m_cbPerDraw.Get(), 0, nullptr, &m_cbPerDrawData, 0, 0);

    m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_deviceContext->IASetInputLayout(shader.InputLayout.Get());

    //- Set Vertex shader stage
    {
        ID3D11Buffer* constantBuffers[]{m_cbPerFrame.Get(), m_cbPerDraw.Get()};
        m_deviceContext->VSSetShader(shader.VertexShader.Get(), nullptr, 0);
        m_deviceContext->VSSetConstantBuffers(0, 2, constantBuffers);
    }
    //- Set Pixel shader stage
    m_deviceContext->PSSetShader(shader.FragmentShader.Get(), nullptr, 0);
}

void DX11Backend::BeginRenderPass(FramebufferHandle framebufferHandle)
{
    const auto& dx11Framebuffer = m_framebuffers[framebufferHandle];

    const auto& dx11DepthStencilRenderTexture = dx11Framebuffer.DepthStencilAttachment;
    const auto  d3dDepthStencilClearFlags     = dx11Framebuffer.DepthStencilClearFlags;

    auto* d3dDepthStencilView = d3dDepthStencilClearFlags != 0
                              ? dx11DepthStencilRenderTexture.DSV.Get()
                              : nullptr;

    //- Set RenderTargets
    {
        const auto& d3dColorRTVs = dx11Framebuffer.ColorRTVs;
        m_deviceContext->OMSetRenderTargets(d3dColorRTVs.size(), d3dColorRTVs.data(), d3dDepthStencilView);
    }
    //- Clear Color attachments
    for (const auto& colorAttachment : dx11Framebuffer.ColorAttachments)
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
            depthStencilClearValue.Stencil);
    }
}

void DX11Backend::EndFrame()
{
    // TODO(v.matushkin): VSync settings
    m_swapChain->Present(0, 0);
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
    // NOTE(v.matushkin): Conversion from ID3D11Device5/ID3D11DeviceContext4 to ID3D11Device/ID3D11DeviceContext
    return new DX11ImGuiRenderContext(m_device.Get(), m_deviceContext.Get());
}


GraphicsState DX11Backend::CreateGraphicsState(const GraphicsStateDesc& graphicsStateDesc)
{
    GraphicsState graphicsState;

    DX11Framebuffer dx11Framebuffer;

    //- Create Color attachments
    for (const auto& colorDesc : graphicsStateDesc.ColorAttachments)
    {
        const auto dxgiColorFormat   = dx11_RenderTextureFormat[static_cast<ui8>(colorDesc.Format)];
        const auto isUsageShaderRead = colorDesc.Usage == RenderTextureUsage::ShaderRead;

        ComPtr<ID3D11Texture2D1>         d3dTexture;
        ComPtr<ID3D11RenderTargetView>   d3dRtv;
        ComPtr<ID3D11ShaderResourceView> d3dSrv = nullptr;

        //-- Create Color texture
        ui32 d3d11TextureBindFlags = D3D11_BIND_RENDER_TARGET;
        if (isUsageShaderRead)
        {
            d3d11TextureBindFlags |= D3D11_BIND_SHADER_RESOURCE;
        }
        D3D11_TEXTURE2D_DESC1 d3dDepthStencilDesc = {
            .Width          = colorDesc.Width,
            .Height         = colorDesc.Height,
            .MipLevels      = 1,
            .ArraySize      = 1,
            .Format         = dxgiColorFormat,
            .SampleDesc     = {.Count = 1, .Quality = 0},
            .Usage          = D3D11_USAGE_DEFAULT,
            .BindFlags      = d3d11TextureBindFlags,
            .CPUAccessFlags = 0,
            .MiscFlags      = 0,                              // NOTE(v.matushkin): Whats this?
            .TextureLayout  = D3D11_TEXTURE_LAYOUT_UNDEFINED, // Can use only UNDEFINED if CPUAccessFlags = 0
        };
        m_device->CreateTexture2D1(&d3dDepthStencilDesc, nullptr, d3dTexture.GetAddressOf());
        //-- Create Color RTV
        // NOTE(v.matushkin): There is a D3D11_RENDER_TARGET_VIEW_DESC1, but it seems useless
        D3D11_RENDER_TARGET_VIEW_DESC d3dRtvDesc = {
            .Format        = dxgiColorFormat,
            .ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D,
            .Texture2D     = {.MipSlice = 0},
        };
        m_device->CreateRenderTargetView(d3dTexture.Get(), &d3dRtvDesc, d3dRtv.GetAddressOf());
        //-- Create Color SRV
        if (isUsageShaderRead)
        {
            D3D11_TEX2D_SRV d3dSrvTex2D = {
                .MostDetailedMip = 0,
                .MipLevels       = 1,
            };
            D3D11_SHADER_RESOURCE_VIEW_DESC d3dSrvDesc = {
                .Format        = dxgiColorFormat,
                .ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
                .Texture2D     = d3dSrvTex2D,
            };
            m_device->CreateShaderResourceView(d3dTexture.Get(), &d3dSrvDesc, d3dSrv.GetAddressOf());
        }

        DX11RenderTexture dx11ColorAttachment = {
            .Texture    = d3dTexture,
            .RTV        = d3dRtv,
            .DSV        = nullptr,
            .SRV        = d3dSrv,
            .ClearValue = colorDesc.ClearValue,
            .LoadAction = colorDesc.LoadAction,
        };

        const auto renderTextureHandle        = static_cast<RenderTextureHandle>(g_RenderTextureHandleWorkaround++);
        m_renderTextures[renderTextureHandle] = dx11ColorAttachment;
        graphicsState.ColorAttachments.push_back(renderTextureHandle);

        dx11Framebuffer.ColorAttachments.push_back(dx11ColorAttachment);
        dx11Framebuffer.ColorRTVs.push_back(d3dRtv.Get());
    }

    //- Create DepthStencil attachment
    dx11Framebuffer.DepthStencilClearFlags = dx11_DepthStencilType[static_cast<ui8>(graphicsStateDesc.DepthStencilType)];

    if (dx11Framebuffer.DepthStencilClearFlags != 0)
    {
        const auto& depthStencilDesc       = graphicsStateDesc.DepthStencilAttachment;
        const auto  dxgiDepthStencilFormat = dx11_RenderTextureFormat[static_cast<ui8>(depthStencilDesc.Format)];

        ComPtr<ID3D11Texture2D1>       d3dTexture;
        ComPtr<ID3D11DepthStencilView> d3dDSV;

        //-- Create DepthStencil texture
        D3D11_TEXTURE2D_DESC1 d3dDepthStencilDesc = {
            .Width          = depthStencilDesc.Width,
            .Height         = depthStencilDesc.Height,
            .MipLevels      = 1,
            .ArraySize      = 1,
            .Format         = dxgiDepthStencilFormat,
            .SampleDesc     = {.Count = 1, .Quality = 0},
            .Usage          = D3D11_USAGE_DEFAULT,
            .BindFlags      = D3D11_BIND_DEPTH_STENCIL,
            .CPUAccessFlags = 0,
            .MiscFlags      = 0,                              // NOTE(v.matushkin): Whats this?
            .TextureLayout  = D3D11_TEXTURE_LAYOUT_UNDEFINED, // Can use only UNDEFINED if CPUAccessFlags = 0
        };
        m_device->CreateTexture2D1(&d3dDepthStencilDesc, nullptr, d3dTexture.GetAddressOf());
        //-- Create DepthStencil view
        D3D11_DEPTH_STENCIL_VIEW_DESC d3dDsvDesc  = {
            .Format        = dxgiDepthStencilFormat,
            .ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D,
            .Flags         = 0,
            .Texture2D     = {.MipSlice = 0},
        };
        m_device->CreateDepthStencilView(d3dTexture.Get(), &d3dDsvDesc, d3dDSV.GetAddressOf());

        dx11Framebuffer.DepthStencilAttachment = {
            .Texture    = d3dTexture,
            .DSV        = d3dDSV,
            .ClearValue = depthStencilDesc.ClearValue,
            .LoadAction = depthStencilDesc.LoadAction,
        };

        const auto renderTextureHandle        = static_cast<RenderTextureHandle>(g_RenderTextureHandleWorkaround++);
        graphicsState.DepthStencilAttachment  = renderTextureHandle;
        m_renderTextures[renderTextureHandle] = dx11Framebuffer.DepthStencilAttachment;
    }

    const auto framebufferHandle      = static_cast<FramebufferHandle>(g_FramebufferHandleWorkaround++);
    graphicsState.Framebuffer         = framebufferHandle;
    m_framebuffers[framebufferHandle] = std::move(dx11Framebuffer);

    return graphicsState;
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
    const auto dxgiTextureFormat     = dx11_TextureFormat[static_cast<ui8>(textureDesc.Format)];
    const auto d3dTextureAddressMode = dx11_TextureWrapMode[static_cast<ui8>(textureDesc.WrapMode)];

    DX11Texture dx11Texture;
    //- Create Texture
    {
        DXGI_SAMPLE_DESC      dxgiSampleDesc = {.Count = 1, .Quality = 0};
        D3D11_TEXTURE2D_DESC1 d3dTextureDesc = {
            .Width          = textureDesc.Width, // TODO(v.matushkin): Make Width/Height ui32?
            .Height         = textureDesc.Height,
            .MipLevels      = 1, // TODO(v.matushkin): No way to use texture without mip, 0 =  generate a full set of subtextures?
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
        D3D11_SAMPLER_DESC d3dSamplerDesc = {
            .Filter         = D3D11_FILTER_MIN_MAG_MIP_POINT,
            .AddressU       = d3dTextureAddressMode,
            .AddressV       = d3dTextureAddressMode,
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

ShaderHandle DX11Backend::CreateShader(std::span<const char> vertexSource, std::span<const char> fragmentSource)
{
    // TODO(v.matushkin): D3DCompile2 ?
    ID3DBlob* d3dVertexBlob;
    ID3DBlob* d3dVertexErrorBlob;
    auto      hr = D3DCompile(
        vertexSource.data(),
        vertexSource.size_bytes(),
        "Vertex Shader Name", // NOTE(v.matushkin): Can be nullptr, don't know where/how it will be used
        nullptr,
        nullptr, // TODO(v.matushkin): I need to use this ID3DInclude
        "main",
        "vs_5_0",
        0, // TODO(v.matushkin): Use this, especially D3DCOMPILE_OPTIMIZATION_LEVEL* and may be D3DCOMPILE_PACK_MATRIX_ROW_MAJOR
        0,
        &d3dVertexBlob,
        &d3dVertexErrorBlob
    );
    if (FAILED(hr))
    {
        // TODO(v.matushkin): For D3DX11CompileFromFile, vertexErrorBlob == nullptr means shader file wasn't found,
        //  so with D3DCompile this cannot happen?
        SNV_ASSERT(d3dVertexErrorBlob != nullptr, "Could this ever happen?");
        LOG_ERROR("Vertex shader compilation error:\n{}", (char*) d3dVertexErrorBlob->GetBufferPointer());
    }
    const auto vertexBuffer     = d3dVertexBlob->GetBufferPointer();
    const auto vertexBufferSize = d3dVertexBlob->GetBufferSize();

    ComPtr<ID3DBlob> d3dFragmentBlob;
    ComPtr<ID3DBlob> d3dFragmentErrorBlob;
    hr = D3DCompile(
        fragmentSource.data(),
        fragmentSource.size_bytes(),
        "Fragment Shader Name",     // NOTE(v.matushkin): Can be nullptr, don't know where/how it will be used
        nullptr,
        nullptr,                    // TODO(v.matushkin): I need to use this ID3DInclude
        "main",
        "ps_5_0",
        0, // TODO(v.matushkin): Use this, especially D3DCOMPILE_OPTIMIZATION_LEVEL* and may be D3DCOMPILE_PACK_MATRIX_ROW_MAJOR
        0,
        d3dFragmentBlob.GetAddressOf(),
        d3dFragmentErrorBlob.GetAddressOf()
    );
    if (FAILED(hr))
    {
        SNV_ASSERT(d3dFragmentErrorBlob != nullptr, "Could this ever happen?");
        LOG_ERROR("Fragment shader compilation error:\n{}", (char*) d3dFragmentErrorBlob->GetBufferPointer());
    }
    const auto fragmentBuffer     = d3dFragmentBlob->GetBufferPointer();
    const auto fragmentBufferSize = d3dFragmentBlob->GetBufferSize();

    DX11Shader dx11Shader;

    m_device->CreateInputLayout(
        k_InputElementDesc,
        ARRAYSIZE(k_InputElementDesc),
        vertexBuffer,
        vertexBufferSize,
        dx11Shader.InputLayout.GetAddressOf()
    );
    m_device->CreateVertexShader(vertexBuffer, vertexBufferSize, nullptr, dx11Shader.VertexShader.GetAddressOf());
    m_device->CreatePixelShader(fragmentBuffer, fragmentBufferSize, nullptr, dx11Shader.FragmentShader.GetAddressOf());

    const auto shaderHandle = static_cast<ShaderHandle>(g_ShaderHandleWorkaround++);
    m_shaders[shaderHandle] = dx11Shader;

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
    const auto& windowSettings = ApplicationSettings::WindowSettings;

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
        .Width       = windowSettings.Width,            // NOTE(v.matushkin): Specify Width/Height explicitly?
        .Height      = windowSettings.Height,
        .Format      = k_SwapchainFormat,
        .Stereo      = false,
        .SampleDesc  = dxgiSwapChainSampleDesc,
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = 2, // TODO(v.matushkin): Hardcoded
        .Scaling     = DXGI_SCALING_STRETCH,            // TODO(v.matushkin): Play with this
        .SwapEffect  = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL,
        .AlphaMode   = DXGI_ALPHA_MODE_IGNORE,          // NOTE(v.matushkin): Don't know
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

    m_swapchainFramebufferHandle                 = static_cast<FramebufferHandle>(g_FramebufferHandleWorkaround++);
    m_framebuffers[m_swapchainFramebufferHandle] = {
        .ColorAttachments       = {colorAttachment},
        .ColorRTVs              = {colorAttachment.RTV.Get()},
        // .DepthStencilAttachment
        .DepthStencilClearFlags = 0,
    };

    //- Setup Viewport
    // TODO(v.matushkin): <RenderGraph/Viewport>
    const auto& graphicsSettings = EngineSettings::GraphicsSettings;

    D3D11_VIEWPORT d3dViewport = {
        .Width    = static_cast<f32>(graphicsSettings.RenderWidth),
        .Height   = static_cast<f32>(graphicsSettings.RenderHeight),
        .MinDepth = 0,
        .MaxDepth = 1,
    };
    m_deviceContext->RSSetViewports(1, &d3dViewport);
}

} // namespace snv
