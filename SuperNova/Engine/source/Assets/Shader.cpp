#include <Engine/Assets/Shader.hpp>
#include <Engine/Renderer/Renderer.hpp>

#include <utility>


namespace snv
{

Shader::Shader(ShaderDesc&& shaderDesc)
    : m_shaderDesc(std::move(shaderDesc))
    , m_shaderHandle(Renderer::CreateShader(m_shaderDesc))
{}

} // namespace snv
