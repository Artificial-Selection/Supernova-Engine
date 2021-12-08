#pragma once

#include <Engine/Renderer/RenderTypes.hpp>


namespace snv
{

namespace RenderDefaults
{
    namespace ShaderState
    {
        inline const snv::BlendState           BlendState =
        {
            .ColorSrcBlendMode = BlendMode::Zero,
            .ColorDstBlendMode = BlendMode::One,
            .AlphaSrcBlendMode = BlendMode::Zero,
            .AlphaDstBlendMode = BlendMode::One,
        };
        inline const snv::CullMode             CullMode             = CullMode::Back;
        inline const snv::DepthCompareFunction DepthCompareFunction = DepthCompareFunction::Greater;
    }
}

} // namespace snv
