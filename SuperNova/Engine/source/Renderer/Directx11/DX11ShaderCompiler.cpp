#include <Engine/Renderer/Directx11/DX11ShaderCompiler.hpp>

#include <Engine/Core/Assert.hpp>
#include <Engine/Renderer/RenderTypes.hpp>
#include <Engine/Renderer/GpuApiCommon/DXCommon.hpp>

#include <d3d11_4.h>
#include <d3dcompiler.h>


// This is the InputLayout for my main shader
// TODO(v.matushkin): Find out how D3D11_APPEND_ALIGNED_ELEMENT works
// TODO(v.matushkin): TEXCOORD to DXGI_FORMAT_R32G32_FLOAT
// static const D3D11_INPUT_ELEMENT_DESC k_InputElementDesc[] = {
//     {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
//     {"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
//     {"TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 2, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
// };

static const char* k_ShaderEntryPoint   = "main";
static const char* k_VertexShaderTarget = "vs_5_0";
static const char* k_PixelShaderTarget  = "ps_5_0";


// DXGI_FORMAT dx11_ComponentTypeToFormat(ui8 mask, D3D_REGISTER_COMPONENT_TYPE d3dComponentType)
// {
//     static const DXGI_FORMAT dxgiComponentFormat[] = {
//         DXGI_FORMAT_UNKNOWN,   // D3D_REGISTER_COMPONENT_UNKNOWN = 0
//         // 1 component
//         DXGI_FORMAT_R32_UINT,  // D3D_REGISTER_COMPONENT_UINT32  = 1
//         DXGI_FORMAT_R32_SINT,  // D3D_REGISTER_COMPONENT_SINT32  = 2
//         DXGI_FORMAT_R32_FLOAT, // D3D_REGISTER_COMPONENT_FLOAT32 = 3
//         // 2 components
//         DXGI_FORMAT_R32G32_UINT,
//         DXGI_FORMAT_R32G32_SINT,
//         DXGI_FORMAT_R32G32_FLOAT,
//         // 3 components
//         DXGI_FORMAT_R32G32B32_UINT,
//         DXGI_FORMAT_R32G32B32_SINT,
//         DXGI_FORMAT_R32G32B32_FLOAT,
//         // 4 components
//         DXGI_FORMAT_R32G32B32A32_UINT,
//         DXGI_FORMAT_R32G32B32A32_SINT,
//         DXGI_FORMAT_R32G32B32A32_FLOAT,
//     };
// 
//     // NOTE(v.matushkin): I think there is no need to check all bits, for example 4 bits for four component type,
//     //  can just check 1st bit for one component, 2nd bit for two component etc.
//     constexpr ui8 oneComponentMask   = 1 << 0;
//     constexpr ui8 twoComponentMask   = 1 << 1 | oneComponentMask;
//     constexpr ui8 threeComponentMask = 1 << 2 | twoComponentMask;
//     constexpr ui8 fourComponentMask  = 1 << 3 | threeComponentMask;
// 
//     i32 lol = 0;
//     if (mask <= oneComponentMask)        lol = 0;
//     else if (mask <= twoComponentMask)   lol = 1;
//     else if (mask <= threeComponentMask) lol = 2;
//     else if (mask <= fourComponentMask)  lol = 3; // NOTE(v.matushkin): Just 'else' ?
// 
//     return dxgiComponentFormat[d3dComponentType + 3 * lol];
// }


namespace snv
{

DX11ShaderCompiler::DX11ShaderCompiler(const ShaderDesc& shaderDesc)
    : m_isImGuiShader(shaderDesc.IsImGuiShader())
{
    m_vertexShaderBlob = CompileShader(shaderDesc.VertexSource, shaderDesc.Name.data(), k_VertexShaderTarget);
    m_pixelShaderBlob  = CompileShader(shaderDesc.FragmentSource, shaderDesc.Name.data(), k_PixelShaderTarget);

    const auto d3dReflectResult = D3DReflect(
        m_vertexShaderBlob->GetBufferPointer(),
        m_vertexShaderBlob->GetBufferSize(),
        IID_PPV_ARGS(&m_vertexShaderReflection)
    );
    if (FAILED(d3dReflectResult))
    {
        LOG_ERROR("D3DReflect failed");
    }
}

DX11ShaderCompiler::~DX11ShaderCompiler()
{
    if (m_vertexShaderBlob != nullptr) m_vertexShaderBlob->Release();
    if (m_pixelShaderBlob != nullptr)  m_pixelShaderBlob->Release();

    if (m_vertexShaderReflection != nullptr) m_vertexShaderReflection->Release();
}


DX11ShaderCompiler::ShaderBuffer DX11ShaderCompiler::GetVertexShaderBuffer() const
{
    return {
        .Ptr  = m_vertexShaderBlob->GetBufferPointer(),
        .Size = m_vertexShaderBlob->GetBufferSize(),
    };
}

DX11ShaderCompiler::ShaderBuffer DX11ShaderCompiler::GetPixelShaderBuffer() const
{
    return {
        .Ptr  = m_pixelShaderBlob->GetBufferPointer(),
        .Size = m_pixelShaderBlob->GetBufferSize(),
    };
}


std::vector<D3D11_INPUT_ELEMENT_DESC> DX11ShaderCompiler::GetInputLayoutDesc() const
{
    //- Get Vertex shader info
    D3D11_SHADER_DESC d3dVertexShaderDesc;
    m_vertexShaderReflection->GetDesc(&d3dVertexShaderDesc);

    //- Read input layout description from vertex shader info
    std::vector<D3D11_INPUT_ELEMENT_DESC> inputLayoutDesc;
    inputLayoutDesc.reserve(d3dVertexShaderDesc.InputParameters);

    for (ui32 i = 0; i < d3dVertexShaderDesc.InputParameters; i++)
    {
        D3D11_SIGNATURE_PARAMETER_DESC d3dSignatureParameterDesc;
        m_vertexShaderReflection->GetInputParameterDesc(i, &d3dSignatureParameterDesc);

        const auto vertexInputAttributeDesc  = dx_VertexInputAttribute(d3dSignatureParameterDesc.SemanticName, m_isImGuiShader);
        const auto dxgiVertexAttributeFormat = dx_VertexAttributeFormat(
            vertexInputAttributeDesc.Format,
            vertexInputAttributeDesc.Dimension
        );

        // NOTE(v.matushkin): Should be careful with SemanticName, probably its lifetime tied to m_vertexShaderReflection
        D3D11_INPUT_ELEMENT_DESC d3dInputElementDesc = {
            .SemanticName         = d3dSignatureParameterDesc.SemanticName,
            .SemanticIndex        = d3dSignatureParameterDesc.SemanticIndex,
            .Format               = dxgiVertexAttributeFormat,
            .InputSlot            = vertexInputAttributeDesc.InputSlot,
            .AlignedByteOffset    = vertexInputAttributeDesc.Offset,
            .InputSlotClass       = D3D11_INPUT_PER_VERTEX_DATA,
            .InstanceDataStepRate = 0,
        };

        inputLayoutDesc.push_back(d3dInputElementDesc);
    }

    return inputLayoutDesc;
}


ID3DBlob* DX11ShaderCompiler::CompileShader(
    const std::string& shaderStageSource,
    const char* shaderName,
    const char* shaderTarget
)
{
    ID3DBlob* d3dShaderStageBlob;
    ID3DBlob* d3dShaderStageErrorBlob;

    // TODO(v.matushkin): D3DCompile2 ?
    const auto d3dCompileResult = D3DCompile(
        shaderStageSource.data(),
        shaderStageSource.length(),
        shaderName,  // NOTE(v.matushkin): Can be nullptr, don't know where/how it will be used
        nullptr,
        nullptr,    // TODO(v.matushkin): I need to use this ID3DInclude
        k_ShaderEntryPoint,
        shaderTarget,
        0,          // TODO(v.matushkin): Use this, especially D3DCOMPILE_OPTIMIZATION_LEVEL* and may be D3DCOMPILE_PACK_MATRIX_ROW_MAJOR
        0,
        &d3dShaderStageBlob,
        &d3dShaderStageErrorBlob
    );
    if (FAILED(d3dCompileResult))
    {
        // TODO(v.matushkin): For D3DX11CompileFromFile, vertexErrorBlob == nullptr means shader file wasn't found,
        //  so with D3DCompile this cannot happen?
        SNV_ASSERT(d3dShaderStageErrorBlob != nullptr, "Could this ever happen?");
        LOG_ERROR("Shader stage compilation error:\n{}", (char*) d3dShaderStageErrorBlob->GetBufferPointer());
        d3dShaderStageErrorBlob->Release();
    }

    return d3dShaderStageBlob;
}

} // namespace snv
