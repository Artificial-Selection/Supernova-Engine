#include <Engine/Assets/Shader.hpp>
#include <Engine/Renderer/Renderer.hpp>


namespace snv
{

Shader::Shader(std::span<const char> vertexSource, std::span<const char> fragmentSource)
    : m_shaderHandle(snv::Renderer::CreateShader(vertexSource, fragmentSource))
{}

} // namespace snv
