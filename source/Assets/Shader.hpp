#pragma once

#include <Core/Core.hpp>
#include <Renderer/RenderTypes.hpp>


namespace snv
{

class Shader
{
public:
    Shader(const char* vertexSource, const char* fragmentSource);

    [[nodiscard]] ShaderHandle GetHandle() const { return m_shaderHandle; }

private:
    ShaderHandle m_shaderHandle;
};

} // namespace snv
