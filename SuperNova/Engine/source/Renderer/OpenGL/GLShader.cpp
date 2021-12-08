#include <Engine/Core/Log.hpp>
#include <Engine/Renderer/OpenGL/GLShader.hpp>

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include <memory>
#include <utility>


namespace snv
{

static_assert((GLenum)GLShaderType::Vertex   == GL_VERTEX_SHADER);
static_assert((GLenum)GLShaderType::Fragment == GL_FRAGMENT_SHADER);


GLShader::GLShader() noexcept
    : m_shaderProgramID(k_InvalidHandle)
{}

GLShader::GLShader(const std::string& vertexSource, const std::string& fragmentSource)
{
    const auto vertexShaderID   = CreateShader(vertexSource.data(), GLShaderType::Vertex);
    const auto fragmentShaderID = CreateShader(fragmentSource.data(), GLShaderType::Fragment);

    CheckShaderCompilationStatus(vertexShaderID);
    CheckShaderCompilationStatus(fragmentShaderID);

    m_shaderProgramID = CreateShaderProgram(vertexShaderID, fragmentShaderID);

    CheckShaderProgramLinkStatus(m_shaderProgramID);

    glDeleteShader(vertexShaderID);
    glDeleteShader(fragmentShaderID);
}

GLShader::GLShader(GLShader&& other) noexcept
    : m_shaderProgramID(std::exchange(other.m_shaderProgramID, k_InvalidHandle))
{}

GLShader& GLShader::operator=(GLShader&& other) noexcept
{
    m_shaderProgramID = std::exchange(other.m_shaderProgramID, k_InvalidHandle);

    return *this;
}


void GLShader::Bind() const
{
    glUseProgram(m_shaderProgramID);
}


void GLShader::SetInt1(const std::string& name, i32 value) const
{
    auto location = glGetUniformLocation(m_shaderProgramID, name.c_str());
    glProgramUniform1i(m_shaderProgramID, location, value);
}

void GLShader::SetMatrix4(const std::string& name, const glm::mat4& value) const
{
    auto location = glGetUniformLocation(m_shaderProgramID, name.c_str());
    glProgramUniformMatrix4fv(m_shaderProgramID, location, 1, GL_FALSE, glm::value_ptr(value));
}


ui32 GLShader::CreateShader(const char* shaderSource, GLShaderType shaderType)
{
    const auto shaderID = glCreateShader(static_cast<GLenum>(shaderType));
    glShaderSource(shaderID, 1, &shaderSource, nullptr);
    glCompileShader(shaderID);

    return shaderID;
}

ui32 GLShader::CreateShaderProgram(i32 vertexShaderID, i32 fragmentShaderID)
{
    ui32 shaderProgramID = glCreateProgram();

    glAttachShader(shaderProgramID, vertexShaderID);
    glAttachShader(shaderProgramID, fragmentShaderID);
    glLinkProgram(shaderProgramID);

    return shaderProgramID;
}


 void GLShader::CheckShaderCompilationStatus(ui32 shaderID)
{
#ifdef SNV_ENABLE_DEBUG

    i32 isCompiled;
    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &isCompiled);

    if (isCompiled == GL_FALSE)
    {
        i32 logLength;
        glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &logLength);

        auto errorLog = std::make_unique<char[]>(logLength);
        glGetShaderInfoLog(shaderID, logLength, nullptr, errorLog.get());
        LOG_ERROR("OpenGL Shader compilation error, Log:\n{)", errorLog.get());
    }

#endif // SNV_ENABLE_DEBUG
}

void GLShader::CheckShaderProgramLinkStatus(ui32 shaderProgramID)
{
#ifdef SNV_ENABLE_DEBUG

    i32 isCompiled;
    glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &isCompiled);

    if (isCompiled == GL_FALSE)
    {
        i32 logLength;
        glGetProgramiv(shaderProgramID, GL_INFO_LOG_LENGTH, &logLength);

        auto errorLog = std::make_unique<char[]>(logLength);
        glGetProgramInfoLog(shaderProgramID, logLength, NULL, errorLog.get());
        LOG_ERROR("OpenGL ShaderProgram link error, Log:\n{)", errorLog.get());
    }

#endif // SNV_ENABLE_DEBUG
}

} // namespace snv
