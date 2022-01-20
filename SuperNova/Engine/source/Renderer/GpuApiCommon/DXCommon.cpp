#include <Engine/Renderer/GpuApiCommon/DXCommon.hpp>

#include <Engine/Core/Assert.hpp>
#include <Engine/Renderer/RenderDefaults.hpp>

#include <dxgiformat.h>

#include <string_view>


namespace SemanticName
{
    static const std::string_view Position = "POSITION";
    static const std::string_view Normal   = "NORMAL";
    static const std::string_view TexCoord = "TEXCOORD";
    static const std::string_view Color    = "COLOR";
}


namespace snv
{

DXGI_FORMAT dx_VertexAttributeFormat(VertexAttributeFormat format, ui8 dimension)
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

// NOTE(v.matushkin): Just a reminder for myself that there are D3D12_APPEND_ALIGNED_ELEMENT/D3D11_APPEND_ALIGNED_ELEMENT for offset
VertexInputAttributeDesc dx_VertexInputAttribute(const char* semanticName, bool isImGuiShader)
{
    VertexInputAttributeDesc vertexInputAttributeDesc;

    if (isImGuiShader)
    {
        if (semanticName == SemanticName::Position)
        {
            vertexInputAttributeDesc = RenderDefaults::ImGuiVertexInputLayout.Position;
        }
        else if (semanticName == SemanticName::TexCoord)
        {
            vertexInputAttributeDesc = RenderDefaults::ImGuiVertexInputLayout.TexCoord0;
        }
        else if (semanticName == SemanticName::Color)
        {
            vertexInputAttributeDesc = RenderDefaults::ImGuiVertexInputLayout.Color;
        }
        else
        {
            // NOTE(v.matushkin): Should log d3dSignatureParameterDesc.SemanticName, but need to modify SNV_ASSERT
            SNV_ASSERT(false, "Unexpected '{}' SemanticName in the ImGui shader");
        }
    }
    else
    {
        if (semanticName == SemanticName::Position)
        {
            vertexInputAttributeDesc = RenderDefaults::EngineVertexInputLayout.Position;
        }
        else if (semanticName == SemanticName::Normal)
        {
            vertexInputAttributeDesc = RenderDefaults::EngineVertexInputLayout.Normal;
        }
        else if (semanticName == SemanticName::TexCoord)
        {
            vertexInputAttributeDesc = RenderDefaults::EngineVertexInputLayout.TexCoord0;
        }
        else
        {
            // NOTE(v.matushkin): Should log d3dSignatureParameterDesc.SemanticName, but need to modify SNV_ASSERT
            SNV_ASSERT(false, "Unsupported '{}' SemanticName in the shader");
        }
    }

    return vertexInputAttributeDesc;
}

bool dx_TriangleFrontFace(TriangleFrontFace triangleFrontFace, bool isImGuiShader)
{
    // For DirectX Clockwise - false, CounterClockwise - true
    return isImGuiShader ? false : triangleFrontFace == TriangleFrontFace::CounterClockwise;
}

} // namespace snv
