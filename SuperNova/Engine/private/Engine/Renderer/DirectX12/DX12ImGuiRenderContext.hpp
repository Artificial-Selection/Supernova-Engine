#pragma once

#include <Engine/Core/Core.hpp>
#include <Engine/Renderer/IImGuiRenderContext.hpp>

#include <d3d12.h>
#include <imgui.h> // NOTE(v.matushkin): Because of ImVec2


struct ImGui_ImplDX12_RenderBuffers;
struct ImGui_ImplDX12_ViewportData;


namespace snv
{

class DX12ImGuiRenderContext final : public IImGuiRenderContext
{
public:
    DX12ImGuiRenderContext(
        ID3D12GraphicsCommandList*  d3dGraphicsCommandList,
        ID3D12Device*               d3dDevice,
        ui32                        numFramesInFlight,
        DXGI_FORMAT                 rtvFormat,
        ID3D12DescriptorHeap*       cbvSrvHeap,
        D3D12_CPU_DESCRIPTOR_HANDLE fontSrvCpuDescHandle,
        D3D12_GPU_DESCRIPTOR_HANDLE fontSrvGpuDescHandle
    );
    ~DX12ImGuiRenderContext() override;

    void BeginFrame() override;
    void EndFrame(ImDrawData* imguiDrawData) override;

private:
    bool ImGui_ImplDX12_Init();
    void ImGui_ImplDX12_Shutdown();
    void ImGui_ImplDX12_NewFrame();
    void ImGui_ImplDX12_RenderDrawData(ImDrawData* draw_data, ID3D12GraphicsCommandList* ctx);

    void ImGui_ImplDX12_CreateFontsTexture();
    bool ImGui_ImplDX12_CreateDeviceObjects();

    void ImGui_ImplDX12_SetupRenderState(ImDrawData* draw_data, ID3D12GraphicsCommandList* ctx, ImGui_ImplDX12_RenderBuffers* fr);
    static void ImGui_ImplDX12_DestroyRenderBuffers(ImGui_ImplDX12_RenderBuffers* render_buffers);
    void ImGui_ImplDX12_InvalidateDeviceObjects();

    // MULTI-VIEWPORT / PLATFORM INTERFACE SUPPORT
    void ImGui_ImplDX12_InitPlatformInterface();
    void ImGui_ImplDX12_ShutdownPlatformInterface();
    static void ImGui_ImplDX12_CreateWindow(ImGuiViewport* viewport);
    static void ImGui_ImplDX12_DestroyWindow(ImGuiViewport* viewport);
    static void ImGui_ImplDX12_SetWindowSize(ImGuiViewport* viewport, ImVec2 size);
    static void ImGui_ImplDX12_RenderWindow(ImGuiViewport* viewport, void*);
    static void ImGui_ImplDX12_SwapBuffers(ImGuiViewport* viewport, void*);

    // For ImGui_ImplDX12_DestroyWindow, ImGui_ImplDX12_SetWindowSize
    static void ImGui_WaitForPendingOperations(ImGui_ImplDX12_ViewportData* vd);

private:
    ID3D12GraphicsCommandList*  m_ctx;
    ID3D12Device*               m_pd3dDevice;
    ID3D12RootSignature*        m_pRootSignature;
    ID3D12PipelineState*        m_pPipelineState;
    DXGI_FORMAT                 m_RTVFormat;
    ID3D12Resource*             m_pFontTextureResource;
    D3D12_CPU_DESCRIPTOR_HANDLE m_hFontSrvCpuDescHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE m_hFontSrvGpuDescHandle;
    ID3D12DescriptorHeap*       m_pd3dSrvDescHeap;
    ui32                        m_numFramesInFlight;
};

} // namespace snv
