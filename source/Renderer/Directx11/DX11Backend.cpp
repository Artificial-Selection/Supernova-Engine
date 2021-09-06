#include <Renderer/Directx11/DX11Backend.hpp>

#include <Application/Window.hpp>
#include <Core/Assert.hpp>

#include <d3d11_4.h>
#include <d3dcompiler.h>

#include <string>


// TODO(v.matushkin): Find out how D3D11_APPEND_ALIGNED_ELEMENT works
// TODO(v.matushkin): TEXCOORD to DXGI_FORMAT_R32G32_FLOAT
constexpr D3D11_INPUT_ELEMENT_DESC k_InputElementDesc[] = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 2, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
};

// NOTE(v.matushkin): Don't know if I should use UINT or UNORM
// TODO(v.matushkin): Apparently there is no support fo 24 bit formats (like RGB8, DEPTH24) on all(almost?) gpus
//  So I guess, I should remove this formats from TextureGraphicsFormat
constexpr DXGI_FORMAT dx11_TextureFormat[] = {
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

constexpr D3D11_TEXTURE_ADDRESS_MODE dx11_TextureWrapMode[] = {
    D3D11_TEXTURE_ADDRESS_CLAMP,
    D3D11_TEXTURE_ADDRESS_BORDER,
    D3D11_TEXTURE_ADDRESS_MIRROR_ONCE,
    D3D11_TEXTURE_ADDRESS_MIRROR,
    D3D11_TEXTURE_ADDRESS_WRAP
};


namespace snv
{

DX11Backend::DX11Backend()
{
    CreateDevice();
    CreateSwapChain();

    CD3D11_BUFFER_DESC cbPerFrameDesc(sizeof(PerFrame), D3D11_BIND_CONSTANT_BUFFER);
    CD3D11_BUFFER_DESC cbPerDrawDesc(sizeof(PerDraw), D3D11_BIND_CONSTANT_BUFFER);

    m_device->CreateBuffer(&cbPerFrameDesc, nullptr, m_cbPerFrame.GetAddressOf());
    m_device->CreateBuffer(&cbPerDrawDesc, nullptr, m_cbPerDraw.GetAddressOf());
}


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

    m_cbPerFrameData._CameraProjection = cameraProjection;
    m_cbPerFrameData._CameraView       = cameraView;
    m_cbPerDrawData._ObjectToWorld     = localToWorld;
    // Update constant buffers
    // TODO(v.matushkin): UpdateSubresource1 ?
    //  And learn what this parameters do
    m_deviceContext->UpdateSubresource(m_cbPerFrame.Get(), 0, nullptr, &m_cbPerFrameData, 0, 0);
    m_deviceContext->UpdateSubresource(m_cbPerDraw.Get(), 0, nullptr, &m_cbPerDrawData, 0, 0);

    // Clear render targets
    m_deviceContext->ClearRenderTargetView(m_renderTargetView.Get(), m_clearColor);
    const auto d3dClearFlags = D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL;
    // TODO(v.matushkin): Hardcoded depth/stencil clear values?
    m_deviceContext->ClearDepthStencilView(m_depthStencilView.Get(), d3dClearFlags, 1.0f, 0);

    m_deviceContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());


    m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_deviceContext->IASetInputLayout(shader.InputLayout.Get());

    // Set up Vertex shader stage
    ID3D11Buffer* constantBuffers[]{m_cbPerFrame.Get(), m_cbPerDraw.Get()};
    m_deviceContext->VSSetShader(shader.VertexShader.Get(), nullptr, 0);
    m_deviceContext->VSSetConstantBuffers(0, 2, constantBuffers);
    // Set up Pixel shader stage
    m_deviceContext->PSSetShader(shader.FragmentShader.Get(), nullptr, 0);
}

void DX11Backend::EndFrame()
{
    m_swapChain->Present(1, 0);
}

void DX11Backend::DrawBuffer(TextureHandle textureHandle, BufferHandle bufferHandle, i32 indexCount, i32 vertexCount)
{
    // TODO(v.matushkin): Rename, there is no GraphicsBuffer anymore
    const auto& graphicsBuffer = m_buffers[bufferHandle];
    const auto& texture        = m_textures[textureHandle];

    ID3D11Buffer* d3dBuffers[] = {graphicsBuffer.Position.Get(), graphicsBuffer.Normal.Get(), graphicsBuffer.TexCoord0.Get()};
    ui32          strides[]    = {sizeof(f32) * 3, sizeof(f32) * 3, sizeof(f32) * 3};
    ui32          offset[]     = {0, 0, 0};

    m_deviceContext->IASetVertexBuffers(0, 3, d3dBuffers, strides, offset);
    m_deviceContext->IASetIndexBuffer(graphicsBuffer.Index.Get(), DXGI_FORMAT_R32_UINT, 0);

    m_deviceContext->PSSetShaderResources(0, 1, texture.SRV.GetAddressOf());
    m_deviceContext->PSSetSamplers(0, 1, texture.Sampler.GetAddressOf());

    m_deviceContext->DrawIndexed(indexCount, 0, 0);
}

void DX11Backend::DrawArrays(i32 count)
{}

void DX11Backend::DrawElements(i32 count)
{}


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

    static ui32 buffer_handle_workaround = 0;
    auto        graphicsBufferHandle     = static_cast<BufferHandle>(buffer_handle_workaround++);

    m_buffers[graphicsBufferHandle] = dx11Buffer;

    return graphicsBufferHandle;
}

TextureHandle DX11Backend::CreateTexture(const TextureDesc& textureDesc, const ui8* textureData)
{
    const auto dxgiTextureFormat     = dx11_TextureFormat[static_cast<ui8>(textureDesc.Format)];
    const auto d3dTextureAddressMode = dx11_TextureWrapMode[static_cast<ui8>(textureDesc.WrapMode)];

    DX11Texture dx11Texture;

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

    static ui32 texture_handle_workaround = 0;
    auto        textureHandle             = static_cast<TextureHandle>(texture_handle_workaround++);

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

    ID3DBlob* d3dFragmentBlob;
    ID3DBlob* d3dFragmentErrorBlob;
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
        &d3dFragmentBlob,
        &d3dFragmentErrorBlob
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

    static ui32 shader_handle_workaround = 0;
    auto        shaderHandle             = static_cast<ShaderHandle>(shader_handle_workaround++);

    m_shaders[shaderHandle] = dx11Shader;

    return shaderHandle;
}


void DX11Backend::CreateDevice()
{
    D3D_FEATURE_LEVEL d3dFeatureLevels[] = {D3D_FEATURE_LEVEL_11_1};

    // NOTE(v.matushkin): Do I even need this?
    //  This flag adds support for surfaces with a color-channel ordering different
    //  from the API default. It is required for compatibility with Direct2D.
    ui32 d3dDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef SNV_GPU_API_DEBUG_ENABLED
    d3dDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif // SNV_GPU_API_DEBUG_ENABLED

    Microsoft::WRL::ComPtr<ID3D11Device>        d3dDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> d3dDeviceContext;

    D3D_FEATURE_LEVEL useless_feature_level_variable;

    D3D11CreateDevice(
        nullptr,                  // Specify nullptr to use the default adapter.
        D3D_DRIVER_TYPE_HARDWARE, // Create a device using the hardware graphics driver.
        0,                        // Should be 0 unless the driver is D3D_DRIVER_TYPE_SOFTWARE.
        d3dDeviceFlags,
        d3dFeatureLevels,
        ARRAYSIZE(d3dFeatureLevels),
        D3D11_SDK_VERSION, // Always set this to D3D11_SDK_VERSION for Windows Store apps. Do I need to?
        d3dDevice.GetAddressOf(),
        &useless_feature_level_variable,
        d3dDeviceContext.GetAddressOf()
    );

    d3dDevice.As(&m_device);
    d3dDeviceContext.As(&m_deviceContext);
}

void DX11Backend::CreateSwapChain()
{
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
        // .Width,                                      // NOTE(v.matushkin): Specify Width/Height explicitly?
        // .Height
        .Format      = DXGI_FORMAT_R8G8B8A8_UNORM,      // TODO(v.matushkin): Shouldn't be hardcoded
        .Stereo      = false,
        .SampleDesc  = dxgiSwapChainSampleDesc,
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = 2,
        .Scaling     = DXGI_SCALING_STRETCH,            // TODO(v.matushkin): Play with this
        .SwapEffect  = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL,
        .AlphaMode   = DXGI_ALPHA_MODE_IGNORE,          // NOTE(v.matushkin): Don't know
        // .Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH  // TODO(v.matushkin): I guess at least I should use this flag?
    };

    Microsoft::WRL::ComPtr<IDXGIDevice4>    dxgiDevice;
    Microsoft::WRL::ComPtr<IDXGIAdapter>    dxgiAdapter;
    Microsoft::WRL::ComPtr<IDXGIAdapter3>   dxgiAdapter3;
    Microsoft::WRL::ComPtr<IDXGIFactory2>   dxgiFactory;
    Microsoft::WRL::ComPtr<IDXGISwapChain1> dxgiSwapChain;

    m_device.As(&dxgiDevice);

    HRESULT result = S_OK;

    result = dxgiDevice->GetAdapter(dxgiAdapter.GetAddressOf());
    dxgiAdapter.As(&dxgiAdapter3);

    dxgiAdapter->GetParent(IID_PPV_ARGS(dxgiFactory.GetAddressOf()));
    dxgiFactory->CreateSwapChainForHwnd(
        m_device.Get(),
        Window::GetWin32Window(),
        &dxgiSwapChainDesc,
        nullptr, // TODO(v.matushkin): Fullscreen swap chain?
        nullptr, // NOTE(v.matushkin): Useless?
        dxgiSwapChain.GetAddressOf()
    );
    dxgiSwapChain.As(&m_swapChain);

    // Get gpu description
    {
        DXGI_ADAPTER_DESC2 dxgiAdapterDesc;
        dxgiAdapter3->GetDesc2(&dxgiAdapterDesc);
        // TODO: Fix this shit with spdlog wide string option support? (need custom conan package then)
        std::wstring wstr(dxgiAdapterDesc.Description);
        std::string  desc(wstr.begin(), wstr.end());
        LOG_INFO("DirectX 11\nAdapter description: {}", desc);
    }

    m_swapChain->GetBuffer(0, IID_PPV_ARGS(m_renderTarget.GetAddressOf()));
    m_device->CreateRenderTargetView(m_renderTarget.Get(), nullptr, m_renderTargetView.GetAddressOf());

    D3D11_TEXTURE2D_DESC renderTargetDesc;
    m_renderTarget->GetDesc(&renderTargetDesc);

    // Create DepthStencil
    DXGI_SAMPLE_DESC      dxgiDepthStencilSampleDesc = {.Count = 1, .Quality = 0};
    D3D11_TEXTURE2D_DESC1 d3dDepthStencilDesc        = {
        .Width          = static_cast<ui32>(renderTargetDesc.Width),
        .Height         = static_cast<ui32>(renderTargetDesc.Height),
        .MipLevels      = 1,
        .ArraySize      = 1,
        .Format         = DXGI_FORMAT_D24_UNORM_S8_UINT,
        .SampleDesc     = dxgiDepthStencilSampleDesc,
        .Usage          = D3D11_USAGE_DEFAULT,
        .BindFlags      = D3D11_BIND_DEPTH_STENCIL,
        .CPUAccessFlags = 0,
        .MiscFlags      = 0,                             // NOTE(v.matushkin): Whats this?
        .TextureLayout  = D3D11_TEXTURE_LAYOUT_UNDEFINED // Can use only UNDEFINED if CPUAccessFlags = 0
    };
    m_device->CreateTexture2D1(&d3dDepthStencilDesc, nullptr, m_depthStencil.GetAddressOf());

    D3D11_TEX2D_DSV               d3dDsvTex2D = {.MipSlice = 0};
    D3D11_DEPTH_STENCIL_VIEW_DESC d3dDsvDesc  = {
        .Format        = DXGI_FORMAT_D24_UNORM_S8_UINT,
        .ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D,
        .Flags         = 0,
        .Texture2D     = d3dDsvTex2D,
    };
    m_device->CreateDepthStencilView(m_depthStencil.Get(), &d3dDsvDesc, m_depthStencilView.GetAddressOf());

    // Setup rasterizer
    D3D11_VIEWPORT d3dViewport = {
        .Width    = static_cast<f32>(renderTargetDesc.Width),
        .Height   = static_cast<f32>(renderTargetDesc.Height),
        .MinDepth = 0,
        .MaxDepth = 1,
    };
    m_deviceContext->RSSetViewports(1, &d3dViewport);

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

} // namespace snv
