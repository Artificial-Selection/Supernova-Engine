#include <Engine/Renderer/DirectX12/DX12ShaderCompiler.hpp>

#include <Engine/Core/Core.hpp>
#include <Engine/Renderer/RenderTypes.hpp>
#include <Engine/Renderer/GpuApiCommon/DXCommon.hpp>

#include <d3d12.h>
#include <dxc/dxcapi.h>
#include <dxc/d3d12shader.h>

#include <string_view>


// This is the InputLayout for my main shader
// static const D3D12_INPUT_ELEMENT_DESC k_InputElementDesc[] = {
//     {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
//     {"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
//     {"TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
// };

static const wchar_t* k_ShaderEntryPoint   = L"main";
static const wchar_t* k_VertexShaderTarget = L"vs_6_5";
static const wchar_t* k_PixelShaderTarget  = L"ps_6_5";

namespace SemanticName
{
    static const std::string_view Position = "POSITION";
    static const std::string_view Normal   = "NORMAL";
    static const std::string_view TexCoord = "TEXCOORD";
    static const std::string_view Color    = "COLOR";
}


namespace snv
{

DX12ShaderCompiler::DX12ShaderCompiler(const ShaderDesc& shaderDesc)
    : m_isImGuiShader(shaderDesc.IsImGuiShader())
{
    // NOTE(v.matushkin): DxcCreateInstance2?
    DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_compiler));
    DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_utils));

    m_vertexShaderBlob = CompileShader(shaderDesc.VertexSource, k_VertexShaderTarget);
    m_pixelShaderBlob  = CompileShader(shaderDesc.FragmentSource, k_PixelShaderTarget);

    m_vertexShaderReflection = CreateReflection(m_vertexShaderBlob);
}

DX12ShaderCompiler::~DX12ShaderCompiler()
{
    if (m_compiler != nullptr)               m_compiler->Release();
    if (m_utils != nullptr)                  m_utils->Release();
    if (m_vertexShaderBlob != nullptr)       m_vertexShaderBlob->Release();
    if (m_vertexShaderBlob != nullptr)       m_pixelShaderBlob->Release();
    if (m_vertexShaderReflection != nullptr) m_vertexShaderReflection->Release();
}


D3D12_SHADER_BYTECODE DX12ShaderCompiler::GetVertexShaderBuffer() const
{
    return {
        .pShaderBytecode = m_vertexShaderBlob->GetBufferPointer(),
        .BytecodeLength  = m_vertexShaderBlob->GetBufferSize(),
    };
}

D3D12_SHADER_BYTECODE DX12ShaderCompiler::GetPixelShaderBuffer() const
{
    return {
        .pShaderBytecode = m_pixelShaderBlob->GetBufferPointer(),
        .BytecodeLength  = m_pixelShaderBlob->GetBufferSize(),
    };
}


std::vector<D3D12_INPUT_ELEMENT_DESC> DX12ShaderCompiler::GetInputLayoutDesc() const
{
    //- Get Vertex shader info
    D3D12_SHADER_DESC d3dVertexShaderDesc;
    m_vertexShaderReflection->GetDesc(&d3dVertexShaderDesc);

    //- Read input layout description from vertex shader info
    std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayoutDesc;
    inputLayoutDesc.reserve(d3dVertexShaderDesc.InputParameters);

    for (ui32 i = 0; i < d3dVertexShaderDesc.InputParameters; ++i)
    {
        D3D12_SIGNATURE_PARAMETER_DESC d3dSignatureParameterDesc;
        m_vertexShaderReflection->GetInputParameterDesc(i, &d3dSignatureParameterDesc);

        const auto vertexInputAttributeDesc  = dx_VertexInputAttribute(d3dSignatureParameterDesc.SemanticName, m_isImGuiShader);
        const auto dxgiVertexAttributeFormat = dx_VertexAttributeFormat(
            vertexInputAttributeDesc.Format,
            vertexInputAttributeDesc.Dimension
        );

        // NOTE(v.matushkin): Should be careful with SemanticName, probably its lifetime tied to m_vertexShaderReflection
        D3D12_INPUT_ELEMENT_DESC d3dInputElementDesc = {
            .SemanticName         = d3dSignatureParameterDesc.SemanticName,
            .SemanticIndex        = d3dSignatureParameterDesc.SemanticIndex,
            .Format               = dxgiVertexAttributeFormat,
            .InputSlot            = vertexInputAttributeDesc.InputSlot,
            .AlignedByteOffset    = vertexInputAttributeDesc.Offset,
            .InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
            .InstanceDataStepRate = 0,
        };

        inputLayoutDesc.push_back(d3dInputElementDesc);
    }

    return inputLayoutDesc;
}


IDxcBlob* DX12ShaderCompiler::CompileShader(const std::string& shaderSource, const wchar_t* profile)
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
        .Ptr      = shaderSource.data(),
        .Size     = shaderSource.length(),
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

    IDxcBlob* dxcShaderBytecode;
    hresult = dxcResult->GetResult(&dxcShaderBytecode);

    return dxcShaderBytecode;
}

// NOTE(v.matushkin): Or use IDxcUtils::CreateReflection() ?
ID3D12ShaderReflection* DX12ShaderCompiler::CreateReflection(IDxcBlob* shaderBlob)
{
    ComPtr<IDxcContainerReflection> dxcContainerReflection;
    DxcCreateInstance(CLSID_DxcContainerReflection, IID_PPV_ARGS(dxcContainerReflection.GetAddressOf()));

    ui32 shaderIdx;
    dxcContainerReflection->Load(shaderBlob);
    dxcContainerReflection->FindFirstPartKind(DXC_PART_DXIL, &shaderIdx);

    ID3D12ShaderReflection* d3dShaderReflection;
    dxcContainerReflection->GetPartReflection(shaderIdx, IID_PPV_ARGS(&d3dShaderReflection));

    return d3dShaderReflection;
}

} // namespace snv
