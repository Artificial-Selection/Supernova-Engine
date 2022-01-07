#pragma once

#include <Engine/Core/Core.hpp>

#include <d3d12.h>
#include <wrl/client.h>

#include <vector>
#include <utility>


namespace snv
{

class DX12DescriptorHeap
{
    template<class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

public:
    DX12DescriptorHeap(ID3D12Device8* d3dDevice);
    ~DX12DescriptorHeap() = default;

    DX12DescriptorHeap(const DX12DescriptorHeap& rhs) = delete;
    DX12DescriptorHeap(DX12DescriptorHeap&& rhs)      = delete;
    DX12DescriptorHeap& operator=(const DX12DescriptorHeap& rhs) = delete;
    DX12DescriptorHeap& operator=(DX12DescriptorHeap&& rhs)      = delete;

    [[nodiscard]] ID3D12DescriptorHeap* GetSRVHeap() const { return m_descriptorHeapSRV.Get(); }

    D3D12_CPU_DESCRIPTOR_HANDLE                                         AllocateRTV();
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>                            AllocateRTV(ui32 numDescriptors);
    D3D12_CPU_DESCRIPTOR_HANDLE                                         AllocateDSV();
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> AllocateSRV();

private:
    // NOTE(v.matushkin): No need to save it right now
    // ID3D12Device8*               m_device; // NOTE(v.matushkin): Use Microsoft::WRL::WeakRef ?

    ComPtr<ID3D12DescriptorHeap> m_descriptorHeapDSV;
    ComPtr<ID3D12DescriptorHeap> m_descriptorHeapRTV;
    ComPtr<ID3D12DescriptorHeap> m_descriptorHeapSRV; // CBV/SRV/UAV

    D3D12_CPU_DESCRIPTOR_HANDLE  m_cpuStartDSV;
    D3D12_CPU_DESCRIPTOR_HANDLE  m_cpuStartRTV;
    D3D12_CPU_DESCRIPTOR_HANDLE  m_cpuStartSRV;
    D3D12_GPU_DESCRIPTOR_HANDLE  m_gpuStartSRV;

    ui64                         m_allocatedDescriptorsDSV;
    ui64                         m_allocatedDescriptorsRTV;
    ui64                         m_allocatedDescriptorsSRV;

    const ui32                   m_descriptorSizeDSV;
    const ui32                   m_descriptorSizeRTV;
    const ui32                   m_descriptorSizeSRV;
};

} // namespace snv
