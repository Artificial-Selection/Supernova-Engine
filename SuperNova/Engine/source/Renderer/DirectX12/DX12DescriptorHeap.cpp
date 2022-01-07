#include <Engine/Renderer/DirectX12/DX12DescriptorHeap.hpp>

#include <Engine/Core/Assert.hpp>


// TODO(v.matushkin): There are a lot of problems with descriptor heap management
// http://diligentgraphics.com/diligent-engine/architecture/d3d12/managing-descriptor-heaps/
// https://docs.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12graphicscommandlist-setdescriptorheaps
// https://stackoverflow.com/questions/37277332/directx-12-descriptor-heaps
// https://stackoverflow.com/questions/37277332/directx-12-descriptor-heaps


// TODO(v.matushkin): <DynamicTextureDescriptorHeap>
namespace MaxDescriptors
{
    static const ui32 DSV      = 1;
    static const ui32 RTV      = 5;
    static const ui32 Textures = 300;
}


namespace snv
{

DX12DescriptorHeap::DX12DescriptorHeap(ID3D12Device8* d3dDevice)
    : m_allocatedDescriptorsDSV(0)
    , m_allocatedDescriptorsRTV(0)
    , m_allocatedDescriptorsSRV(0)
    , m_descriptorSizeDSV(d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV))
    , m_descriptorSizeRTV(d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV))
    , m_descriptorSizeSRV(d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV))
{
    //- Create DSV heap
    D3D12_DESCRIPTOR_HEAP_DESC d3dDescriptorHeapDSV = {
        .Type           = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
        .NumDescriptors = MaxDescriptors::DSV,
        .Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE, // This flag only applies to CBV, SRV, UAV and samplers
        .NodeMask       = 0,
    };
    d3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDSV, IID_PPV_ARGS(m_descriptorHeapDSV.GetAddressOf()));
    //- Create RTV heap
    D3D12_DESCRIPTOR_HEAP_DESC d3dDescriptorHeapRTV = {
        .Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        .NumDescriptors = MaxDescriptors::RTV,
        .Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        .NodeMask       = 0,
    };
    d3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapRTV, IID_PPV_ARGS(m_descriptorHeapRTV.GetAddressOf()));
    //- Create SRV heap
    D3D12_DESCRIPTOR_HEAP_DESC d3dDescriptorHeapSRV = {
        .Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        .NumDescriptors = MaxDescriptors::Textures,
        .Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        .NodeMask       = 0,
    };
    d3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapSRV, IID_PPV_ARGS(m_descriptorHeapSRV.GetAddressOf()));

    //- Get first DescriprtorHandles
    m_cpuStartDSV = m_descriptorHeapDSV->GetCPUDescriptorHandleForHeapStart();
    m_cpuStartRTV = m_descriptorHeapRTV->GetCPUDescriptorHandleForHeapStart();
    m_cpuStartSRV = m_descriptorHeapSRV->GetCPUDescriptorHandleForHeapStart();
    m_gpuStartSRV = m_descriptorHeapSRV->GetGPUDescriptorHandleForHeapStart();
}


D3D12_CPU_DESCRIPTOR_HANDLE DX12DescriptorHeap::AllocateRTV()
{
    SNV_ASSERT(m_allocatedDescriptorsRTV + 1 <= MaxDescriptors::RTV, "Exceeded RTV descriptors limit");

    auto d3dDescriptorHandle = m_cpuStartRTV;
    d3dDescriptorHandle.ptr += m_descriptorSizeRTV * m_allocatedDescriptorsRTV;

    m_allocatedDescriptorsRTV++;

    return d3dDescriptorHandle;
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12DescriptorHeap::AllocateDSV()
{
    SNV_ASSERT(m_allocatedDescriptorsDSV + 1 <= MaxDescriptors::DSV, "Exceeded DSV descriptors limit");

    auto d3dDescriptorHandle = m_cpuStartDSV;
    d3dDescriptorHandle.ptr += m_descriptorSizeDSV * m_allocatedDescriptorsDSV;

    m_allocatedDescriptorsDSV++;

    return d3dDescriptorHandle;
}

std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> DX12DescriptorHeap::AllocateRTV(ui32 numDescriptors)
{
    SNV_ASSERT(m_allocatedDescriptorsRTV + numDescriptors <= MaxDescriptors::RTV, "Exceeded RTV descriptors limit");

    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> d3dDescriptorHandles;
    d3dDescriptorHandles.reserve(numDescriptors);

    auto currentFreeDescriptor = m_cpuStartRTV;
    currentFreeDescriptor.ptr += m_descriptorSizeRTV * m_allocatedDescriptorsRTV;

    for (ui32 i = 0; i < numDescriptors; ++i)
    {
        d3dDescriptorHandles.push_back(currentFreeDescriptor);
        currentFreeDescriptor.ptr += m_descriptorSizeRTV;
    }

    m_allocatedDescriptorsRTV += numDescriptors;

    return d3dDescriptorHandles;
}

std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> DX12DescriptorHeap::AllocateSRV()
{
    SNV_ASSERT(m_allocatedDescriptorsSRV + 1 <= MaxDescriptors::Textures, "Exceeded SRV descriptors limit");

    auto d3dCpuDescriptorHandle = m_cpuStartSRV;
    d3dCpuDescriptorHandle.ptr += m_descriptorSizeSRV * m_allocatedDescriptorsSRV;

    auto d3dGpuDescriptorHandle = m_gpuStartSRV;
    d3dGpuDescriptorHandle.ptr += m_descriptorSizeSRV * m_allocatedDescriptorsSRV;

    m_allocatedDescriptorsSRV++;

    return {d3dCpuDescriptorHandle, d3dGpuDescriptorHandle};
}

} // namespace snv
