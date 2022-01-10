#pragma once

#include <Engine/Renderer/RenderTypes.hpp>


namespace snv
{

namespace RenderDefaults
{
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
}

} // namespace snv
