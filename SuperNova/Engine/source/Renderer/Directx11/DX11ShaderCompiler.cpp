#include <Engine/Renderer/Directx11/DX11ShaderCompiler.hpp>

#include <Engine/Core/Assert.hpp>
#include <Engine/Renderer/RenderDefaults.hpp>
#include <Engine/Renderer/RenderTypes.hpp>

#include <d3d11_4.h>
#include <d3dcompiler.h>

#include <string_view>


// This is an InputLayout for my main shader
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

namespace SemanticName
{
    static const std::string_view Position = "POSITION";
    static const std::string_view Normal   = "NORMAL";
    static const std::string_view TexCoord = "TEXCOORD";
    static const std::string_view Color    = "COLOR";
}


DXGI_FORMAT dx11_VertexAttributeFormat(snv::VertexAttributeFormat format, ui8 dimension)
{
    static const DXGI_FORMAT dxgiVertexFormat[] = {
        // SInt8
        DXGI_FORMAT_R8_SINT,
        DXGI_FORMAT_R8G8_SINT,
        DXGI_FORMAT_UNKNOWN,
        DXGI_FORMAT_R8G8B8A8_SINT,
        // SInt16
        DXGI_FORMAT_R16_SINT,
        DXGI_FORMAT_R16G16_SINT,
        DXGI_FORMAT_UNKNOWN,
        DXGI_FORMAT_R16G16B16A16_SINT,
        // SInt32
        DXGI_FORMAT_R32_SINT,
        DXGI_FORMAT_R32G32_SINT,
        DXGI_FORMAT_R32G32B32_SINT,
        DXGI_FORMAT_R32G32B32A32_SINT,
        // UInt8
        DXGI_FORMAT_R8_UINT,
        DXGI_FORMAT_R8G8_UINT,
        DXGI_FORMAT_UNKNOWN,
        DXGI_FORMAT_R8G8B8A8_UINT,
        // UInt16
        DXGI_FORMAT_R16_UINT,
        DXGI_FORMAT_R16G16_UINT,
        DXGI_FORMAT_UNKNOWN,
        DXGI_FORMAT_R16G16B16A16_UINT,
        // UInt32
        DXGI_FORMAT_R32_UINT,
        DXGI_FORMAT_R32G32_UINT,
        DXGI_FORMAT_R32G32B32_UINT,
        DXGI_FORMAT_R32G32B32A32_UINT,
        // SNorm8
        DXGI_FORMAT_R8_SNORM,
        DXGI_FORMAT_R8G8_SNORM,
        DXGI_FORMAT_UNKNOWN,
        DXGI_FORMAT_R8G8B8A8_SNORM,
        // SNorm16
        DXGI_FORMAT_R16_SNORM,
        DXGI_FORMAT_R16G16_SNORM,
        DXGI_FORMAT_UNKNOWN,
        DXGI_FORMAT_R16G16B16A16_SNORM,
        // UNorm8
        DXGI_FORMAT_R8_UNORM,
        DXGI_FORMAT_R8G8_UNORM,
        DXGI_FORMAT_UNKNOWN,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        // UNorm16
        DXGI_FORMAT_R16_UNORM,
        DXGI_FORMAT_R16G16_UNORM,
        DXGI_FORMAT_UNKNOWN,
        DXGI_FORMAT_R16G16B16A16_UNORM,
        // Float16
        DXGI_FORMAT_R16_FLOAT,
        DXGI_FORMAT_R16G16_FLOAT,
        DXGI_FORMAT_UNKNOWN,
        DXGI_FORMAT_R16G16B16A16_FLOAT,
        // Float32
        DXGI_FORMAT_R32_FLOAT,
        DXGI_FORMAT_R32G32_FLOAT,
        DXGI_FORMAT_R32G32B32_FLOAT,
        DXGI_FORMAT_R32G32B32A32_FLOAT,
    };

    SNV_ASSERT(dimension >= 1 && dimension <= 4, "Vertex attribute dimension must be in [1,4] range");
    const auto dxgiFormat = dxgiVertexFormat[static_cast<ui8>(format) * 4 + dimension - 1];
    SNV_ASSERT(dxgiFormat != DXGI_FORMAT_UNKNOWN, "Unsupported vertex format/dimension combination");

    return dxgiFormat;
}

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
    : m_isImGuiShader(shaderDesc.Name == RenderDefaults::ImGuiShaderName)
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
    D3D11_SHADER_DESC d3dShaderDesc;
    m_vertexShaderReflection->GetDesc(&d3dShaderDesc);

    //- Read input layout description from vertex shader info
    std::vector<D3D11_INPUT_ELEMENT_DESC> inputLayoutDesc;
    inputLayoutDesc.reserve(d3dShaderDesc.InputParameters);

    for (ui32 i = 0; i < d3dShaderDesc.InputParameters; i++)
    {
        D3D11_SIGNATURE_PARAMETER_DESC d3dSignatureParameterDesc;
        m_vertexShaderReflection->GetInputParameterDesc(i, &d3dSignatureParameterDesc);

        VertexAttributeFormat vertexAttributeFormat;
        ui8                   vertexAttributeDimension;
        ui8                   inputSlot;
        ui32                  alignedByteOffset;

        if (m_isImGuiShader)
        {
            // NOTE(v.matushkin): Can I just use D3D11_APPEND_ALIGNED_ELEMENT for alignedByteOffset ?
            inputSlot = 0;

            if (d3dSignatureParameterDesc.SemanticName == SemanticName::Position)
            {
                vertexAttributeFormat    = RenderDefaults::ImGuiVertexInputLayout.PositionFormat;
                vertexAttributeDimension = RenderDefaults::ImGuiVertexInputLayout.PositionDimension;
                alignedByteOffset        = RenderDefaults::ImGuiVertexInputLayout.PositionOffset;
            }
            else if (d3dSignatureParameterDesc.SemanticName == SemanticName::TexCoord)
            {
                vertexAttributeFormat    = RenderDefaults::ImGuiVertexInputLayout.TexCoor0Format;
                vertexAttributeDimension = RenderDefaults::ImGuiVertexInputLayout.TexCoor0Dimension;
                alignedByteOffset        = RenderDefaults::ImGuiVertexInputLayout.TexCoor0Offset;
            }
            else if (d3dSignatureParameterDesc.SemanticName == SemanticName::Color)
            {
                vertexAttributeFormat    = RenderDefaults::ImGuiVertexInputLayout.ColorFormat;
                vertexAttributeDimension = RenderDefaults::ImGuiVertexInputLayout.ColorDimension;
                alignedByteOffset        = RenderDefaults::ImGuiVertexInputLayout.ColorOffset;
            }
            else
            {
                // NOTE(v.matushkin): Should log d3dSignatureParameterDesc.SemanticName, but need to modify SNV_ASSERT
                SNV_ASSERT(false, "Unexpected '{}' SemanticName in the ImGui shader");
            }
        }
        else
        {
            alignedByteOffset = 0;

            if (d3dSignatureParameterDesc.SemanticName == SemanticName::Position)
            {
                vertexAttributeFormat    = RenderDefaults::EngineVertexInputLayout.PositionFormat;
                vertexAttributeDimension = RenderDefaults::EngineVertexInputLayout.PositionDimension;
                inputSlot                = RenderDefaults::EngineVertexInputLayout.PositionStream;
            }
            else if (d3dSignatureParameterDesc.SemanticName == SemanticName::Normal)
            {
                vertexAttributeFormat    = RenderDefaults::EngineVertexInputLayout.NormalFormat;
                vertexAttributeDimension = RenderDefaults::EngineVertexInputLayout.NormalDimension;
                inputSlot                = RenderDefaults::EngineVertexInputLayout.NormalStream;
            }
            else if (d3dSignatureParameterDesc.SemanticName == SemanticName::TexCoord)
            {
                vertexAttributeFormat    = RenderDefaults::EngineVertexInputLayout.TexCoor0Format;
                vertexAttributeDimension = RenderDefaults::EngineVertexInputLayout.TexCoor0Dimension;
                inputSlot                = RenderDefaults::EngineVertexInputLayout.TexCoor0Stream;
            }
            else
            {
                // NOTE(v.matushkin): Should log d3dSignatureParameterDesc.SemanticName, but need to modify SNV_ASSERT
                SNV_ASSERT(false, "Unsupported '{}' SemanticName in the shader");
            }
        }

        // NOTE(v.matushkin): Should be careful with SemanticName, probably its lifetime tied to m_vertexShaderReflection
        D3D11_INPUT_ELEMENT_DESC d3dInputElementDesc = {
            .SemanticName         = d3dSignatureParameterDesc.SemanticName,
            .SemanticIndex        = d3dSignatureParameterDesc.SemanticIndex,
            .Format               = dx11_VertexAttributeFormat(vertexAttributeFormat, vertexAttributeDimension),
            .InputSlot            = inputSlot,
            .AlignedByteOffset    = alignedByteOffset,
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
