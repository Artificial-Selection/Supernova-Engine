#pragma once

#include <Engine/Renderer/RenderTypes.hpp>

#include <string_view>


namespace snv::RenderDefaults
{
    inline const std::string_view ImGuiShaderName = "Engine/ImGui";


    struct EngineInputLayout
    {
        VertexAttributeFormat PositionFormat;
        ui8                   PositionDimension;
        ui8                   PositionStream; // NOTE(v.matushkin): Stream/InputSlot/Location/Binding which is the good name?
        VertexAttributeFormat NormalFormat;
        ui8                   NormalDimension;
        ui8                   NormalStream;
        VertexAttributeFormat TexCoor0Format;
        ui8                   TexCoor0Dimension;
        ui8                   TexCoor0Stream;
    };
    struct ImGuiInputLayout
    {
        VertexAttributeFormat PositionFormat;
        ui8                   PositionDimension;
        ui8                   PositionOffset;
        VertexAttributeFormat TexCoor0Format;
        ui8                   TexCoor0Dimension;
        ui8                   TexCoor0Offset;
        VertexAttributeFormat ColorFormat;
        ui8                   ColorDimension;
        ui8                   ColorOffset;
    };

    inline const EngineInputLayout EngineVertexInputLayout = {
        .PositionFormat    = VertexAttributeFormat::Float32,
        .PositionDimension = 3,
        .PositionStream    = 0,
        .NormalFormat      = VertexAttributeFormat::Float32,
        .NormalDimension   = 3,
        .NormalStream      = 1,
        .TexCoor0Format    = VertexAttributeFormat::Float32,
        .TexCoor0Dimension = 3,
        .TexCoor0Stream    = 2,
    };

    inline const ImGuiInputLayout ImGuiVertexInputLayout = {
        .PositionFormat    = VertexAttributeFormat::Float32,
        .PositionDimension = 2,
        .PositionOffset    = 0,
        .TexCoor0Format    = VertexAttributeFormat::Float32,
        .TexCoor0Dimension = 2,
        .TexCoor0Offset    = 8,
        .ColorFormat       = VertexAttributeFormat::UNorm8,
        .ColorDimension    = 4,
        .ColorOffset       = 16,
    };


    // NOTE(v.matushkin): Shoud be in BlendStateDesc
    inline const bool IndependentBlendEnable = false; // NOTE(v.matushkin): Need a lot of checks to support it


    inline const RasterizerStateDesc   RasterizerState = {
        .PolygonMode = PolygonMode::Fill,
        .CullMode    = CullMode::Back,
        .FrontFace   = TriangleFrontFace::CounterClockwise,
    };

    inline const DepthStencilStateDesc DepthStencilState = {
        .DepthTestEnable      = true,
        .DepthWriteEnable     = true,
        .DepthCompareFunction = CompareFunction::Less,
    };

    inline const BlendStateDesc        BlendState =
    {
        .BlendMode           = BlendMode::Off,
        .ColorSrcBlendFactor = BlendFactor::One,
        .ColorDstBlendFactor = BlendFactor::Zero,
        .ColorBlendOp        = BlendOp::Add,
        .AlphaSrcBlendFactor = BlendFactor::One,
        .AlphaDstBlendFactor = BlendFactor::Zero,
        .AlphaBlendOp        = BlendOp::Add,
        .LogicOp             = BlendLogicOp::Noop,
    };

} // namespace snv::RenderDefaults
