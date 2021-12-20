#pragma once

#include <Engine/Core/Core.hpp>
#include <Engine/Renderer/IImGuiRenderContext.hpp>


enum DXGI_FORMAT;
struct ID3D12Device;
struct ID3D12DescriptorHeap;
struct ID3D12GraphicsCommandList;
struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct D3D12_GPU_DESCRIPTOR_HANDLE;


namespace snv
{

class DX12ImGuiRenderContext final: public IImGuiRenderContext
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
    ID3D12GraphicsCommandList* m_cmd; // For ImGui_ImplDX12_RenderDrawData
};

} // namespace snv
