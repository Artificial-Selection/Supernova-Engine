#include <Assets/Shader.hpp>
#include <Renderer/Renderer.hpp>


namespace snv
{

Shader::Shader(const char* vertexSource, const char* fragmentSource)
{
    m_shaderHandle = snv::Renderer::CreateShader(vertexSource, fragmentSource);
}

} // namespace snv
