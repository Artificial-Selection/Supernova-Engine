#include <Core/Log.hpp>
#include <Renderer/OpenGL/GLShader.hpp>

#include <glad/glad.h>

#include <memory>


static_assert((GLenum)GLShaderType::Vertex == GL_VERTEX_SHADER);
static_assert((GLenum)GLShaderType::Fragment == GL_FRAGMENT_SHADER);


GLShader::GLShader( const char* const* vertexSource, const char* const* fragmentSource )
{
    const auto vertexShaderID = CreateShader( vertexSource, GLShaderType::Vertex );
    const auto fragmentShaderID = CreateShader( fragmentSource, GLShaderType::Fragment );

    CheckShaderCompilationStatus( vertexShaderID );
    CheckShaderCompilationStatus( fragmentShaderID );

    m_shaderProgramID = CreateShaderProgram( vertexShaderID, fragmentShaderID );

    CheckShaderProgramLinkStatus( m_shaderProgramID );

    glDeleteShader( vertexShaderID );
    glDeleteShader( fragmentShaderID );
}


ui32 GLShader::CreateShader( const char* const* shaderSource, GLShaderType shaderType )
{
    const auto shaderID = glCreateShader( static_cast<GLenum>(shaderType) );
    glShaderSource( shaderID, 1, shaderSource, nullptr );
    glCompileShader( shaderID );

    return shaderID;
}

ui32 GLShader::CreateShaderProgram( i32 vertexShaderID, i32 fragmentShaderID )
{
    ui32 shaderProgramID = glCreateProgram();

    glAttachShader( shaderProgramID, vertexShaderID );
    glAttachShader( shaderProgramID, fragmentShaderID );
    glLinkProgram( shaderProgramID );

    return shaderProgramID;
}


void GLShader::CheckShaderCompilationStatus( ui32 shaderID )
{
#ifdef SNV_ENABLE_DEBUG

    i32 isCompiled;
    glGetShaderiv( shaderID, GL_COMPILE_STATUS, &isCompiled );

    if (isCompiled == GL_FALSE) {
        i32 logLength;
        glGetShaderiv( shaderID, GL_INFO_LOG_LENGTH, &logLength );

        auto errorLog = std::make_unique<char[]>( logLength );
        glGetShaderInfoLog( shaderID, logLength, nullptr, errorLog.get() );
        LOG_ERROR( "OpenGL Shader compilation error, Log:\n{)", errorLog.get() );
    }

#endif // SNV_ENABLE_DEBUG
}

void GLShader::CheckShaderProgramLinkStatus( ui32 shaderProgramID )
{
#ifdef SNV_ENABLE_DEBUG

    i32 isCompiled;
    glGetProgramiv( shaderProgramID, GL_LINK_STATUS, &isCompiled );

    if (isCompiled == GL_FALSE) {
        i32 logLength;
        glGetProgramiv( shaderProgramID, GL_INFO_LOG_LENGTH, &logLength );

        auto errorLog = std::make_unique<char[]>( logLength );
        glGetProgramInfoLog( shaderProgramID, logLength, NULL, errorLog.get() );
        LOG_ERROR( "OpenGL ShaderProgram link error, Log:\n{)", errorLog.get() );
    }

#endif // SNV_ENABLE_DEBUG
}
