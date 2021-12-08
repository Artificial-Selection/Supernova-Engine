#include <Engine/Renderer/RenderTypes.hpp>
#include <Engine/Renderer/RenderDefaults.hpp>


namespace snv
{

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
