#include <Engine/Renderer/Directx11/DX11ImGuiRenderContext.hpp>

#include <imgui_impl_dx11.h>


namespace snv
{

DX11ImGuiRenderContext::DX11ImGuiRenderContext(ID3D11Device* d3dDevice, ID3D11DeviceContext* d3dDeviceContext)
{
    ImGui_ImplDX11_Init(d3dDevice, d3dDeviceContext);
}

DX11ImGuiRenderContext::~DX11ImGuiRenderContext()
{
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

} // namespace snv
