#pragma once

#include <Engine/Renderer/RenderTypes.hpp>

#include <string_view>


namespace snv::RenderDefaults
{
    inline const std::string_view ImGuiShaderName = "Engine/ImGui";

    // NOTE(v.matushkin): Not sure about this structs naming
    struct EngineInputLayout
    {
        VertexInputAttributeDesc Position;
        VertexInputAttributeDesc Normal;
        VertexInputAttributeDesc TexCoord0;
    };
    struct ImGuiInputLayout
    {
        VertexInputAttributeDesc Position;
        VertexInputAttributeDesc TexCoord0;
        VertexInputAttributeDesc Color;
    };

    inline const EngineInputLayout EngineVertexInputLayout = {
        .Position = {
            .Offset    = 0,
            .Format    = VertexAttributeFormat::Float32,
            .Dimension = 3,
            .InputSlot = 0,
        },
        .Normal = {
            .Offset    = 0,
            .Format    = VertexAttributeFormat::Float32,
            .Dimension = 3,
            .InputSlot = 1,
        },
        .TexCoord0 = {
            .Offset    = 0,
            .Format    = VertexAttributeFormat::Float32,
            .Dimension = 3,
            .InputSlot = 2,
        },
    };

    inline const ImGuiInputLayout ImGuiVertexInputLayout = {
        .Position = {
            .Offset    = 0,
            .Format    = VertexAttributeFormat::Float32,
            .Dimension = 2,
            .InputSlot = 0,
        },
        .TexCoord0 = {
            .Offset    = 8,
            .Format    = VertexAttributeFormat::Float32,
            .Dimension = 2,
            .InputSlot = 0,
        },
        .Color = {
            .Offset    = 16,
            .Format    = VertexAttributeFormat::UNorm8,
            .Dimension = 4,
            .InputSlot = 0,
        },
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
