#pragma once

#include <Core/Core.hpp>


enum class GLShaderType
{
    Vertex   = 0x8B31,
    Fragment = 0x8B30
};

class GLShader
{
public:
    GLShader( const char* const* vertexSource, const char* const* fragmentSource );

private:
    static ui32 CreateShader( const char* const* shaderSource, GLShaderType shaderType );
    static ui32 CreateShaderProgram( i32 vertexShaderID, i32 fragmentShaderID );

    static void CheckShaderCompilationStatus( ui32 shaderID );
    static void CheckShaderProgramLinkStatus( ui32 shaderProgramID );

private:
    ui32 m_shaderProgramID;
};
