#pragma once

#include <Core/Core.hpp>
#include <Renderer/RenderTypes.hpp>

#include <span>


namespace snv
{

class Shader
{
public:
    Shader(std::span<const char> vertexSource, std::span<const char> fragmentSource);

    [[nodiscard]] ShaderHandle GetHandle() const { return m_shaderHandle; }

private:
    ShaderHandle m_shaderHandle;
};

} // namespace snv
