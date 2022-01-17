#pragma once

#include <Engine/Core/Core.hpp>
#include <Engine/Renderer/IImGuiRenderContext.hpp>
#include <Engine/Renderer/RenderTypes.hpp>

#include <imgui.h> // NOTE(v.matushkin): Because of ImVec2


struct IDXGIFactory5;
struct ID3D11Device5;
struct ID3D11DeviceContext4;
struct ID3D11Buffer;
struct ID3D11Buffer;
struct ID3D11Buffer;
struct ID3D11SamplerState;
struct ID3D11ShaderResourceView;


namespace snv
{

class DX11Backend;


class DX11ImGuiRenderContext final : public IImGuiRenderContext
{
public:
    DX11ImGuiRenderContext(
        DX11Backend*          dx11Backend,
        ID3D11Device5*        d3dDevice,
        ID3D11DeviceContext4* d3dDeviceContext,
        IDXGIFactory5*        d3dFactory
    );
    ~DX11ImGuiRenderContext() override;

    void BeginFrame() override;
    void EndFrame(ImDrawData* imguiDrawData) override;

private:
    bool ImGui_ImplDX11_Init();
    void ImGui_ImplDX11_Shutdown();
    void ImGui_ImplDX11_NewFrame();
    void ImGui_ImplDX11_RenderDrawData(ImDrawData* draw_data);

    void ImGui_ImplDX11_CreateFontsTexture();
    bool ImGui_ImplDX11_CreateDeviceObjects();

    // For ImGui_ImplOpenGL3_RenderDrawData
    void ImGui_ImplDX11_SetupRenderState(ImDrawData* draw_data);
    // For ImGui_ImplDX11_Shutdown, ImGui_ImplDX11_CreateDeviceObjects
    void ImGui_ImplDX11_InvalidateDeviceObjects();

    // MULTI-VIEWPORT / PLATFORM INTERFACE SUPPORT
    void ImGui_ImplDX11_InitPlatformInterface();
    void ImGui_ImplDX11_ShutdownPlatformInterface();
    static void ImGui_ImplDX11_CreateWindow(ImGuiViewport* viewport);
    static void ImGui_ImplDX11_DestroyWindow(ImGuiViewport* viewport);
    static void ImGui_ImplDX11_SetWindowSize(ImGuiViewport* viewport, ImVec2 size);
    static void ImGui_ImplDX11_RenderWindow(ImGuiViewport* viewport, void*);
    static void ImGui_ImplDX11_SwapBuffers(ImGuiViewport* viewport, void*);

private:
    DX11Backend*              m_dx11Backend;
    ShaderHandle              m_imguiShaderHandle;

    ID3D11Device5*            m_pd3dDevice;
    ID3D11DeviceContext4*     m_pd3dDeviceContext;
    IDXGIFactory5*            m_pFactory;
    ID3D11Buffer*             m_pVB;
    ID3D11Buffer*             m_pIB;
    ID3D11Buffer*             m_pVertexConstantBuffer;
    ID3D11SamplerState*       m_pFontSampler;
    ID3D11ShaderResourceView* m_pFontTextureView;
    i32                       m_VertexBufferSize;
    i32                       m_IndexBufferSize;
};

} // namespace snv
