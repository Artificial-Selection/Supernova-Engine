#pragma once

#include <Core/Core.hpp>
#include <Renderer/RenderTypes.hpp>

#include <glm/ext/matrix_float4x4.hpp>

#include <string>


namespace snv
{

enum class GLShaderType
{
    Vertex   = 0x8B31,
    Fragment = 0x8B30
};


class GLShader
{
public:
    // NOTE(v.matushkin): Can I make this move only without default constructor?
    // TODO(v.matushkin): Define destructor
    GLShader() noexcept;
    GLShader(const char* vertexSource, const char* fragmentSource);

    GLShader(GLShader&& other) noexcept;
    GLShader& operator=(GLShader&& other) noexcept;

    GLShader(const GLShader& other) = delete;
    GLShader& operator=(const GLShader& other) = delete;

    [[nodiscard]] ShaderHandle GetHandle() const { return static_cast<ShaderHandle>(m_shaderProgramID); }

    void Bind() const;

    void SetInt1(const std::string& name, i32 value) const;
    void SetMatrix4(const std::string& name, const glm::mat4& value) const;

private:
    static ui32 CreateShader(const char* shaderSource, GLShaderType shaderType);
    static ui32 CreateShaderProgram(i32 vertexShaderID, i32 fragmentShaderID);

    static void CheckShaderCompilationStatus(ui32 shaderID);
    static void CheckShaderProgramLinkStatus(ui32 shaderProgramID);

private:
    ui32 m_shaderProgramID;
};

}  // namespace snv
