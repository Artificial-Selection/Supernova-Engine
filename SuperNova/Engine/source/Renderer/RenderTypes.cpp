#include <Engine/Renderer/RenderTypes.hpp>
#include <Engine/Renderer/RenderDefaults.hpp>


namespace snv
{

RenderTextureType RenderTextureDesc::RenderTextureType() const
{
    if (Format == RenderTextureFormat::BGRA32)
    {
        return RenderTextureType::Color;
    }

    return RenderTextureType::Depth;
}

ShaderState ShaderState::Default()
{
    return ShaderState
    {
        .BlendState           = RenderDefaults::ShaderState::BlendState,
        .CullMode             = RenderDefaults::ShaderState::CullMode,
        .DepthCompareFunction = RenderDefaults::ShaderState::DepthCompareFunction,
    };
}

} // namespace snv
