#include <Engine/Renderer/Directx12/DX12ImGuiRenderContext.hpp>

#include <d3d12.h>
#include <imgui_impl_dx12.h>


namespace snv
{

DX12ImGuiRenderContext::DX12ImGuiRenderContext(
    ID3D12GraphicsCommandList*  d3dGraphicsCommandList,
    ID3D12Device*               d3dDevice,
    ui32                        numFramesInFlight,
    DXGI_FORMAT                 rtvFormat,
    ID3D12DescriptorHeap*       cbvSrvHeap,
    D3D12_CPU_DESCRIPTOR_HANDLE fontSrvCpuDescHandle,
    D3D12_GPU_DESCRIPTOR_HANDLE fontSrvGpuDescHandle
)
    : m_cmd(d3dGraphicsCommandList)
{
    ImGui_ImplDX12_Init(d3dDevice, numFramesInFlight, rtvFormat, cbvSrvHeap, fontSrvCpuDescHandle, fontSrvGpuDescHandle);
}

DX12ImGuiRenderContext::~DX12ImGuiRenderContext()
{
    ImGui_ImplDX12_Shutdown();
}


void DX12ImGuiRenderContext::BeginFrame()
{
    ImGui_ImplDX12_NewFrame();
}

void DX12ImGuiRenderContext::EndFrame(ImDrawData* imguiDrawData)
{
    ImGui_ImplDX12_RenderDrawData(imguiDrawData, m_cmd);
}

} // namespace snv
