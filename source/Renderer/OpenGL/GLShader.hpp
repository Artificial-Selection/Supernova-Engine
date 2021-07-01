#pragma once

#include <Core/Core.hpp>


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
    GLShader(const char* vertexSource, const char* fragmentSource);

    void Bind() const;

private:
    static ui32 CreateShader(const char* shaderSource, GLShaderType shaderType);
    static ui32 CreateShaderProgram(i32 vertexShaderID, i32 fragmentShaderID);

    static void CheckShaderCompilationStatus(ui32 shaderID);
    static void CheckShaderProgramLinkStatus(ui32 shaderProgramID);

private:
    ui32 m_shaderProgramID;
};

}  // namespace snv
