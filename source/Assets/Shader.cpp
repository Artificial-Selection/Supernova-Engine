#include <Assets/Shader.hpp>
#include <Renderer/Renderer.hpp>


namespace snv
{

Shader::Shader(std::span<const char> vertexSource, std::span<const char> fragmentSource)
    : m_shaderHandle(snv::Renderer::CreateShader(vertexSource, fragmentSource))
{}

} // namespace snv
