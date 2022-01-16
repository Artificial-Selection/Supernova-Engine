#include <Engine/Renderer/DirectX12/DX12ShaderCompiler.hpp>

#include <Engine/Renderer/RenderTypes.hpp>

#include <d3d12.h>

// #include <vector>


static const wchar_t* k_ShaderEntryPoint   = L"main";
static const wchar_t* k_VertexShaderTarget = L"vs_6_5";
static const wchar_t* k_PixelShaderTarget  = L"ps_6_5";


namespace snv
{

DX12ShaderCompiler::DX12ShaderCompiler()
{
    // NOTE(v.matushkin): DxcCreateInstance2?
    DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(m_compiler.GetAddressOf()));
    DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(m_utils.GetAddressOf()));
}


DX12ShaderCompiler::DX12ShaderDesc DX12ShaderCompiler::CompileShader(const ShaderDesc& shaderDesc)
{
    return DX12ShaderDesc{
        .VertexBytecode = CompileShaderStage(k_VertexShaderTarget, shaderDesc.VertexSource),
        .PixelBytecode  = CompileShaderStage(k_PixelShaderTarget, shaderDesc.FragmentSource),
    };
}


Microsoft::WRL::ComPtr<IDxcBlob> DX12ShaderCompiler::CompileShaderStage(const wchar_t* profile, const std::string& shaderStageSource)
{
    // https://developer.nvidia.com/dx12-dos-and-donts
    // Use the /all_resources_bound / D3DCOMPILE_ALL_RESOURCES_BOUND compile flag if possible
    // This allows for the compiler to do a better job at optimizing texture accesses. We have
    // seen frame rate improvements of > 1 % when toggling this flag on.
    LPCWSTR arguments[] = {
        DXC_ARG_ALL_RESOURCES_BOUND,
    };

    ComPtr<IDxcCompilerArgs> dxcCompilerArgs;
    HRESULT hresult = m_utils->BuildArguments(
        nullptr,
        k_ShaderEntryPoint,
        profile,
        arguments,
        ARRAYSIZE(arguments),
        nullptr,
        0,
        dxcCompilerArgs.GetAddressOf()
    );

    DxcBuffer dxcBuffer = {
        .Ptr      = shaderStageSource.data(),
        .Size     = shaderStageSource.length(),
        .Encoding = 0, //NOTE(v.matushkin): ??? CP_UTF8 ??? DXC_CP_UTF8 ???
    };

    ComPtr<IDxcResult> dxcResult;
    hresult = m_compiler->Compile(
        &dxcBuffer,
        dxcCompilerArgs->GetArguments(),
        dxcCompilerArgs->GetCount(),
        nullptr,
        IID_PPV_ARGS(dxcResult.GetAddressOf())
    );

    dxcResult->GetStatus(&hresult);
    if (FAILED(hresult))
    {
        ComPtr<IDxcBlobEncoding> dxcErrors;
        dxcResult->GetErrorBuffer(dxcErrors.GetAddressOf());
        OutputDebugStringA((LPSTR)dxcErrors->GetBufferPointer());
    }

    ComPtr<IDxcBlob> dxcShaderBytecode;
    hresult = dxcResult->GetResult(dxcShaderBytecode.GetAddressOf());

    return dxcShaderBytecode;
}

// ID3D12ShaderReflection* DX12ShaderCompiler::CreateReflection(IDxcBlob* shaderBlob)
// {
//     ComPtr<IDxcContainerReflection> dxcContainerReflection;
//     DxcCreateInstance(CLSID_DxcContainerReflection, IID_PPV_ARGS(dxcContainerReflection.GetAddressOf()));
// 
//     ui32 shaderIdx;
//     dxcContainerReflection->Load(shaderBlob);
//     dxcContainerReflection->FindFirstPartKind(DXC_PART_DXIL, &shaderIdx);
// 
//     ID3D12ShaderReflection* d3dShaderReflection;
//     dxcContainerReflection->GetPartReflection(shaderIdx, IID_PPV_ARGS(&d3dShaderReflection));
// 
//     return d3dShaderReflection;
// }

// void Lol(ID3D12ShaderReflection* d3dVertexShaderReflection)
// {
//     D3D12_SHADER_DESC d3dVertexShaderDesc;
//     d3dVertexShaderReflection->GetDesc(&d3dVertexShaderDesc);
// 
//     std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayoutDesc;
//     inputLayoutDesc.reserve(d3dVertexShaderDesc.InputParameters);
// 
//     for (ui32 i = 0; i < d3dVertexShaderDesc.InputParameters; ++i)
//     {
//         D3D12_SIGNATURE_PARAMETER_DESC d3dSignatureParameterDesc;
//         d3dVertexShaderReflection->GetInputParameterDesc(i, &d3dSignatureParameterDesc);
// 
//         D3D12_INPUT_ELEMENT_DESC d3dInputElementDesc = {
//             .SemanticName         = d3dSignatureParameterDesc.SemanticName,
//             .SemanticIndex        = d3dSignatureParameterDesc.SemanticIndex,
//             .Format               = ,
//             .InputSlot            = i,
//             .AlignedByteOffset    = 0,
//             .InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
//             .InstanceDataStepRate = 0,
//         };
// 
//         inputLayoutDesc.push_back(d3dInputElementDesc);
//     }
// 
//     auto cbRefl = d3dVertexShaderReflection->GetConstantBufferByIndex(0);
//     D3D12_SHADER_BUFFER_DESC d3dShaderBufferDesc;
//     cbRefl->GetDesc(&d3dShaderBufferDesc);
// 
// }

} // namespace snv
