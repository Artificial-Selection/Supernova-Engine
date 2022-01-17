#include <Engine/Renderer/Directx11/DX11ImGuiRenderContext.hpp>

#include <Engine/Assets/AssetDatabase.hpp>
#include <Engine/Assets/Shader.hpp>
#include <Engine/Renderer/Directx11/DX11Backend.hpp>

#include <d3d11_4.h>
#include <stdio.h>


struct VERTEX_CONSTANT_BUFFER
{
    f32 mvp[4][4];
};


// Helper structure we store in the void* RenderUserData field of each ImGuiViewport to easily retrieve our backend data.
struct ImGui_ImplDX11_ViewportData
{
    IDXGISwapChain*                 SwapChain;
    ID3D11RenderTargetView*         RTView;

    ImGui_ImplDX11_ViewportData()   { SwapChain = nullptr; RTView = nullptr; }
    ~ImGui_ImplDX11_ViewportData()  { IM_ASSERT(SwapChain == nullptr && RTView == nullptr); }
};


namespace snv
{

static DX11ImGuiRenderContext* g_imguiRenderContext; // NOTE(v.matushkin): Hack


DX11ImGuiRenderContext::DX11ImGuiRenderContext(
    DX11Backend*          dx11Backend,
    ID3D11Device5*        d3dDevice,
    ID3D11DeviceContext4* d3dDeviceContext,
    IDXGIFactory5*        d3dFactory
)
    : m_dx11Backend(dx11Backend)
    , m_imguiShaderHandle(ShaderHandle::InvalidHandle)

    , m_pd3dDevice(d3dDevice)
    , m_pd3dDeviceContext(d3dDeviceContext)
    , m_pFactory(d3dFactory)
    , m_pVB(nullptr)
    , m_pIB(nullptr)
    , m_pVertexConstantBuffer(nullptr)
    , m_pFontSampler(nullptr)
    , m_pFontTextureView(nullptr)
    , m_VertexBufferSize(0)
    , m_IndexBufferSize(0)
{
    g_imguiRenderContext = this;
    ImGui_ImplDX11_Init();
}

DX11ImGuiRenderContext::~DX11ImGuiRenderContext()
{
    g_imguiRenderContext = nullptr;
    ImGui_ImplDX11_Shutdown();
}


void DX11ImGuiRenderContext::BeginFrame()
{
    ImGui_ImplDX11_NewFrame();
}

void DX11ImGuiRenderContext::EndFrame(ImDrawData* imguiDrawData)
{
    ImGui_ImplDX11_RenderDrawData(imguiDrawData);
}


bool DX11ImGuiRenderContext::ImGui_ImplDX11_Init()
{
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.BackendRendererUserData == nullptr && "Already initialized a renderer backend!");

    // Setup backend capabilities flags
    // io.BackendRendererUserData = (void*)bd;
    io.BackendRendererName = "DX11ImGuiRenderContext";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
    io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;  // We can create multi-viewports on the Renderer side (optional)

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        ImGui_ImplDX11_InitPlatformInterface();

    return true;
}

void DX11ImGuiRenderContext::ImGui_ImplDX11_Shutdown()
{
    ImGui_ImplDX11_ShutdownPlatformInterface();
    ImGui_ImplDX11_InvalidateDeviceObjects();

    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName     = nullptr;
    io.BackendRendererUserData = nullptr;
}

void DX11ImGuiRenderContext::ImGui_ImplDX11_NewFrame()
{
    if (!m_pFontSampler)
        ImGui_ImplDX11_CreateDeviceObjects();
}

void DX11ImGuiRenderContext::ImGui_ImplDX11_RenderDrawData(ImDrawData* draw_data)
{
    // Avoid rendering when minimized
    if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
        return;

    ID3D11DeviceContext4* ctx = m_pd3dDeviceContext;

    // Create and grow vertex/index buffers if needed
    if (m_pVB == nullptr || m_VertexBufferSize < draw_data->TotalVtxCount)
    {
        if (m_pVB) { m_pVB->Release(); m_pVB = nullptr; }
        m_VertexBufferSize = draw_data->TotalVtxCount + 5000;
        D3D11_BUFFER_DESC desc;
        memset(&desc, 0, sizeof(D3D11_BUFFER_DESC));
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.ByteWidth = m_VertexBufferSize * sizeof(ImDrawVert);
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        desc.MiscFlags = 0;
        if (m_pd3dDevice->CreateBuffer(&desc, nullptr, &m_pVB) < 0)
            return;
    }
    if (m_pIB == nullptr || m_IndexBufferSize < draw_data->TotalIdxCount)
    {
        if (m_pIB) { m_pIB->Release(); m_pIB = nullptr; }
        m_IndexBufferSize = draw_data->TotalIdxCount + 10000;
        D3D11_BUFFER_DESC desc;
        memset(&desc, 0, sizeof(D3D11_BUFFER_DESC));
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.ByteWidth = m_IndexBufferSize * sizeof(ImDrawIdx);
        desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        if (m_pd3dDevice->CreateBuffer(&desc, nullptr, &m_pIB) < 0)
            return;
    }

    // Upload vertex/index data into a single contiguous GPU buffer
    D3D11_MAPPED_SUBRESOURCE vtx_resource, idx_resource;
    if (ctx->Map(m_pVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &vtx_resource) != S_OK)
        return;
    if (ctx->Map(m_pIB, 0, D3D11_MAP_WRITE_DISCARD, 0, &idx_resource) != S_OK)
        return;
    ImDrawVert* vtx_dst = (ImDrawVert*)vtx_resource.pData;
    ImDrawIdx* idx_dst = (ImDrawIdx*)idx_resource.pData;
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
        vtx_dst += cmd_list->VtxBuffer.Size;
        idx_dst += cmd_list->IdxBuffer.Size;
    }
    ctx->Unmap(m_pVB, 0);
    ctx->Unmap(m_pIB, 0);

    // Setup orthographic projection matrix into our constant buffer
    // Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right).
    // DisplayPos is (0,0) for single viewport apps.
    {
        D3D11_MAPPED_SUBRESOURCE mapped_resource;
        if (ctx->Map(m_pVertexConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource) != S_OK)
            return;
        VERTEX_CONSTANT_BUFFER* constant_buffer = (VERTEX_CONSTANT_BUFFER*)mapped_resource.pData;
        float L = draw_data->DisplayPos.x;
        float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
        float T = draw_data->DisplayPos.y;
        float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
        float mvp[4][4] =
        {
            { 2.0f/(R-L),   0.0f,           0.0f,       0.0f },
            { 0.0f,         2.0f/(T-B),     0.0f,       0.0f },
            { 0.0f,         0.0f,           0.5f,       0.0f },
            { (R+L)/(L-R),  (T+B)/(B-T),    0.5f,       1.0f },
        };
        memcpy(&constant_buffer->mvp, mvp, sizeof(mvp));
        ctx->Unmap(m_pVertexConstantBuffer, 0);
    }

    // Backup DX state that will be modified to restore it afterwards (unfortunately this is very ugly looking and verbose. Close your eyes!)
    struct BACKUP_DX11_STATE
    {
        UINT                      ScissorRectsCount, ViewportsCount;
        D3D11_RECT                ScissorRects[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
        D3D11_VIEWPORT            Viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
        ID3D11ShaderResourceView* PSShaderResource;
        ID3D11SamplerState*       PSSampler;
        ID3D11Buffer*             IndexBuffer, *VertexBuffer, *VSConstantBuffer;
        UINT                      IndexBufferOffset, VertexBufferStride, VertexBufferOffset;
        DXGI_FORMAT               IndexBufferFormat;
    };
    BACKUP_DX11_STATE old = {};
    old.ScissorRectsCount = old.ViewportsCount = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    ctx->RSGetScissorRects(&old.ScissorRectsCount, old.ScissorRects);
    ctx->RSGetViewports(&old.ViewportsCount, old.Viewports);
    ctx->PSGetShaderResources(0, 1, &old.PSShaderResource);
    ctx->PSGetSamplers(0, 1, &old.PSSampler);
    ctx->VSGetConstantBuffers(0, 1, &old.VSConstantBuffer);

    ctx->IAGetIndexBuffer(&old.IndexBuffer, &old.IndexBufferFormat, &old.IndexBufferOffset);
    ctx->IAGetVertexBuffers(0, 1, &old.VertexBuffer, &old.VertexBufferStride, &old.VertexBufferOffset);

    // Setup desired DX state
    ImGui_ImplDX11_SetupRenderState(draw_data);

    // Render command lists
    // (Because we merged all buffers into a single one, we maintain our own offset into them)
    int global_idx_offset = 0;
    int global_vtx_offset = 0;
    ImVec2 clip_off = draw_data->DisplayPos;
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback != nullptr)
            {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                    ImGui_ImplDX11_SetupRenderState(draw_data);
                else
                    pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec2 clip_min(pcmd->ClipRect.x - clip_off.x, pcmd->ClipRect.y - clip_off.y);
                ImVec2 clip_max(pcmd->ClipRect.z - clip_off.x, pcmd->ClipRect.w - clip_off.y);
                if (clip_max.x < clip_min.x || clip_max.y < clip_min.y)
                    continue;

                // Apply scissor/clipping rectangle
                const D3D11_RECT r = { (LONG)clip_min.x, (LONG)clip_min.y, (LONG)clip_max.x, (LONG)clip_max.y };
                ctx->RSSetScissorRects(1, &r);

                // Bind texture, Draw
                ID3D11ShaderResourceView* texture_srv = (ID3D11ShaderResourceView*)pcmd->GetTexID();
                ctx->PSSetShaderResources(0, 1, &texture_srv);
                ctx->DrawIndexed(pcmd->ElemCount, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset);
            }
        }
        global_idx_offset += cmd_list->IdxBuffer.Size;
        global_vtx_offset += cmd_list->VtxBuffer.Size;
    }

    // Restore modified DX state
    ctx->RSSetScissorRects(old.ScissorRectsCount, old.ScissorRects);
    ctx->RSSetViewports(old.ViewportsCount, old.Viewports);
    ctx->PSSetShaderResources(0, 1, &old.PSShaderResource);
    ctx->PSSetSamplers(0, 1, &old.PSSampler);
    ctx->VSSetConstantBuffers(0, 1, &old.VSConstantBuffer);
    ctx->IASetIndexBuffer(old.IndexBuffer, old.IndexBufferFormat, old.IndexBufferOffset);
    ctx->IASetVertexBuffers(0, 1, &old.VertexBuffer, &old.VertexBufferStride, &old.VertexBufferOffset);

    if (old.PSShaderResource) old.PSShaderResource->Release();
    if (old.PSSampler)        old.PSSampler->Release();
    if (old.VSConstantBuffer) old.VSConstantBuffer->Release();
    if (old.IndexBuffer)      old.IndexBuffer->Release();
    if (old.VertexBuffer)     old.VertexBuffer->Release();
}


void DX11ImGuiRenderContext::ImGui_ImplDX11_CreateFontsTexture()
{
    // Build texture atlas
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    // Upload texture to graphics system
    {
        D3D11_TEXTURE2D_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;

        ID3D11Texture2D* pTexture = nullptr;
        D3D11_SUBRESOURCE_DATA subResource;
        subResource.pSysMem = pixels;
        subResource.SysMemPitch = desc.Width * 4;
        subResource.SysMemSlicePitch = 0;
        m_pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);
        IM_ASSERT(pTexture != nullptr);

        // Create texture view
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        ZeroMemory(&srvDesc, sizeof(srvDesc));
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = desc.MipLevels;
        srvDesc.Texture2D.MostDetailedMip = 0;
        m_pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, &m_pFontTextureView);
        pTexture->Release();
    }

    // Store our identifier
    io.Fonts->SetTexID((ImTextureID) m_pFontTextureView);

    // Create texture sampler
    {
        D3D11_SAMPLER_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.MipLODBias = 0.f;
        desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
        desc.MinLOD = 0.f;
        desc.MaxLOD = 0.f;
        m_pd3dDevice->CreateSamplerState(&desc, &m_pFontSampler);
    }
}

bool DX11ImGuiRenderContext::ImGui_ImplDX11_CreateDeviceObjects()
{
    if (m_pd3dDevice == nullptr)
        return false;
    if (m_pFontSampler != nullptr)
        ImGui_ImplDX11_InvalidateDeviceObjects();

    // Create the constant buffer
    {
        D3D11_BUFFER_DESC desc;
        desc.ByteWidth      = sizeof(VERTEX_CONSTANT_BUFFER);
        desc.Usage          = D3D11_USAGE_DYNAMIC;
        desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        desc.MiscFlags      = 0;
        m_pd3dDevice->CreateBuffer(&desc, nullptr, &m_pVertexConstantBuffer);
    }

    const auto imguiShader = AssetDatabase::LoadAsset<Shader>("ImGui");
    m_imguiShaderHandle    = imguiShader->GetHandle();

    ImGui_ImplDX11_CreateFontsTexture();

    return true;
}


void DX11ImGuiRenderContext::ImGui_ImplDX11_SetupRenderState(ImDrawData* draw_data)
{
    m_dx11Backend->BindShader(m_imguiShaderHandle);

    ID3D11DeviceContext4* ctx = m_pd3dDeviceContext;

    // Setup viewport
    D3D11_VIEWPORT vp;
    memset(&vp, 0, sizeof(D3D11_VIEWPORT));
    vp.Width = draw_data->DisplaySize.x;
    vp.Height = draw_data->DisplaySize.y;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = vp.TopLeftY = 0;
    ctx->RSSetViewports(1, &vp);

    // Setup shader and vertex buffers
    unsigned int stride = sizeof(ImDrawVert);
    unsigned int offset = 0;
    ctx->IASetVertexBuffers(0, 1, &m_pVB, &stride, &offset);
    ctx->IASetIndexBuffer(m_pIB, sizeof(ImDrawIdx) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, 0);
    ctx->VSSetConstantBuffers(0, 1, &m_pVertexConstantBuffer);
    ctx->PSSetSamplers(0, 1, &m_pFontSampler);

    // NOTE(v.matushkin): Does blend_factor matters? It's different from the DX11Backend::BindShader()
    // Setup blend state
    // const float blend_factor[4] = { 0.f, 0.f, 0.f, 0.f };
    // ctx->OMSetBlendState(m_pBlendState, blend_factor, 0xffffffff);
}

void DX11ImGuiRenderContext::ImGui_ImplDX11_InvalidateDeviceObjects()
{
    if (m_pd3dDevice == nullptr)
        return;

    if (m_pFontSampler)          { m_pFontSampler->Release();          m_pFontSampler = nullptr; }
    // We copied data->pFontTextureView to io.Fonts->TexID so let's clear that as well.
    if (m_pFontTextureView)      { m_pFontTextureView->Release();      m_pFontTextureView = nullptr; ImGui::GetIO().Fonts->SetTexID(nullptr); }
    if (m_pIB)                   { m_pIB->Release();                   m_pIB = nullptr; }
    if (m_pVB)                   { m_pVB->Release();                   m_pVB = nullptr; }
    if (m_pVertexConstantBuffer) { m_pVertexConstantBuffer->Release(); m_pVertexConstantBuffer = nullptr; }
}


void DX11ImGuiRenderContext::ImGui_ImplDX11_InitPlatformInterface()
{
    ImGuiPlatformIO& platform_io       = ImGui::GetPlatformIO();
    platform_io.Renderer_CreateWindow  = ImGui_ImplDX11_CreateWindow;
    platform_io.Renderer_DestroyWindow = ImGui_ImplDX11_DestroyWindow;
    platform_io.Renderer_SetWindowSize = ImGui_ImplDX11_SetWindowSize;
    platform_io.Renderer_RenderWindow  = ImGui_ImplDX11_RenderWindow;
    platform_io.Renderer_SwapBuffers   = ImGui_ImplDX11_SwapBuffers;
}

void DX11ImGuiRenderContext::ImGui_ImplDX11_ShutdownPlatformInterface()
{
    ImGui::DestroyPlatformWindows();
}

void DX11ImGuiRenderContext::ImGui_ImplDX11_CreateWindow(ImGuiViewport* viewport)
{
    DX11ImGuiRenderContext*      bd = g_imguiRenderContext;
    ImGui_ImplDX11_ViewportData* vd = IM_NEW(ImGui_ImplDX11_ViewportData)();
    viewport->RendererUserData = vd;

    // PlatformHandleRaw should always be a HWND, whereas PlatformHandle might be a higher-level handle (e.g. GLFWWindow*, SDL_Window*).
    // Some backend will leave PlatformHandleRaw NULL, in which case we assume PlatformHandle will contain the HWND.
    HWND hwnd = viewport->PlatformHandleRaw ? (HWND)viewport->PlatformHandleRaw : (HWND)viewport->PlatformHandle;
    IM_ASSERT(hwnd != 0);

    // Create swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferDesc.Width = (UINT)viewport->Size.x;
    sd.BufferDesc.Height = (UINT)viewport->Size.y;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 1;
    sd.OutputWindow = hwnd;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    sd.Flags = 0;

    IM_ASSERT(vd->SwapChain == NULL && vd->RTView == NULL);
    bd->m_pFactory->CreateSwapChain(bd->m_pd3dDevice, &sd, &vd->SwapChain);

    // Create the render target
    if (vd->SwapChain)
    {
        ID3D11Texture2D* pBackBuffer;
        vd->SwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
        bd->m_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &vd->RTView);
        pBackBuffer->Release();
    }
}

void DX11ImGuiRenderContext::ImGui_ImplDX11_DestroyWindow(ImGuiViewport* viewport)
{
    // The main viewport (owned by the application) will always have RendererUserData == NULL since we didn't create the data for it.
    if (ImGui_ImplDX11_ViewportData* vd = (ImGui_ImplDX11_ViewportData*)viewport->RendererUserData)
    {
        if (vd->SwapChain)
            vd->SwapChain->Release();
        vd->SwapChain = nullptr;
        if (vd->RTView)
            vd->RTView->Release();
        vd->RTView = nullptr;
        IM_DELETE(vd);
    }
    viewport->RendererUserData = nullptr;
}

void DX11ImGuiRenderContext::ImGui_ImplDX11_SetWindowSize(ImGuiViewport* viewport, ImVec2 size)
{
    DX11ImGuiRenderContext*      bd = g_imguiRenderContext;
    ImGui_ImplDX11_ViewportData* vd = (ImGui_ImplDX11_ViewportData*)viewport->RendererUserData;
    if (vd->RTView)
    {
        vd->RTView->Release();
        vd->RTView = nullptr;
    }
    if (vd->SwapChain)
    {
        ID3D11Texture2D* pBackBuffer = nullptr;
        vd->SwapChain->ResizeBuffers(0, (UINT)size.x, (UINT)size.y, DXGI_FORMAT_UNKNOWN, 0);
        vd->SwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
        if (pBackBuffer == nullptr) { fprintf(stderr, "ImGui_ImplDX11_SetWindowSize() failed creating buffers.\n"); return; }
        bd->m_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &vd->RTView);
        pBackBuffer->Release();
    }
}

void DX11ImGuiRenderContext::ImGui_ImplDX11_RenderWindow(ImGuiViewport* viewport, void*)
{
    DX11ImGuiRenderContext*      bd = g_imguiRenderContext;
    ImGui_ImplDX11_ViewportData* vd = (ImGui_ImplDX11_ViewportData*)viewport->RendererUserData;
    ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    bd->m_pd3dDeviceContext->OMSetRenderTargets(1, &vd->RTView, nullptr);
    if (!(viewport->Flags & ImGuiViewportFlags_NoRendererClear))
        bd->m_pd3dDeviceContext->ClearRenderTargetView(vd->RTView, (float*)&clear_color);

    bd->ImGui_ImplDX11_RenderDrawData(viewport->DrawData);
}

void DX11ImGuiRenderContext::ImGui_ImplDX11_SwapBuffers(ImGuiViewport* viewport, void*)
{
    ImGui_ImplDX11_ViewportData* vd = (ImGui_ImplDX11_ViewportData*)viewport->RendererUserData;
    vd->SwapChain->Present(0, 0); // Present without vsync
}

} // namespace snv
