#pragma once

#include <Engine/Core/Core.hpp>
#include <Engine/Renderer/RenderTypes.hpp>


namespace snv
{

class Shader
{
public:
    Shader(ShaderDesc&& shaderDesc);

    [[nodiscard]] ShaderHandle GetHandle() const { return m_shaderHandle; }

private:
    const ShaderDesc m_shaderDesc;

    ShaderHandle m_shaderHandle;
};

} // namespace snv
