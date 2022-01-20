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

RasterizerStateDesc RasterizerStateDesc::Default()
{
    return RenderDefaults::RasterizerState;
}

DepthStencilStateDesc DepthStencilStateDesc::Default()
{
    return RenderDefaults::DepthStencilState;
}

BlendStateDesc BlendStateDesc::Default()
{
    return RenderDefaults::BlendState;
}

bool ShaderDesc::IsImGuiShader() const
{
    return Name == RenderDefaults::ImGuiShaderName;
}

} // namespace snv
