#include <Renderer/OpenGL/GLBackend.hpp>
#include <Core/Log.hpp>

#include <glad/glad.h>


static_assert((GLenum)GLBlendFactor::One == GL_ONE);
static_assert((GLenum)GLBlendFactor::SrcAlpha == GL_SRC_ALPHA);
static_assert((GLenum)GLBlendFactor::OneMinusSrcAlpha == GL_ONE_MINUS_SRC_ALPHA);

static_assert((GLenum)GLDepthFunction::Never == GL_NEVER);
static_assert((GLenum)GLDepthFunction::Less == GL_LESS);
static_assert((GLenum)GLDepthFunction::Equal == GL_EQUAL);
static_assert((GLenum)GLDepthFunction::LessOrEqual == GL_LEQUAL);
static_assert((GLenum)GLDepthFunction::Greater == GL_GREATER);
static_assert((GLenum)GLDepthFunction::NotEqual == GL_NOTEQUAL);
static_assert((GLenum)GLDepthFunction::GreaterOrEqual == GL_GEQUAL);
static_assert((GLenum)GLDepthFunction::Always == GL_ALWAYS);

static_assert((GLenum)GLBufferBit::Color == GL_COLOR_BUFFER_BIT);
static_assert((GLenum)GLBufferBit::Depth == GL_DEPTH_BUFFER_BIT);
//static_assert((GLenum)GLBufferBit::Accum == GL_ACCUM_BUFFER_BIT); // WTF IT'S UNDEFINED?
static_assert((GLenum)GLBufferBit::Stencil == GL_STENCIL_BUFFER_BIT);


#ifdef SNV_ENABLE_DEBUG
void openGLMessageCallback(GLenum source, GLenum type,
                           ui32 id, GLenum severity, GLsizei length,
                           const char* message, const void* userParam)
{
    const char* messageSource;
    switch (source) {
        case GL_DEBUG_SOURCE_API:             messageSource = "API"; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   messageSource = "Window System"; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: messageSource = "Shader Compiler"; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:     messageSource = "Third Party"; break;
        case GL_DEBUG_SOURCE_APPLICATION:     messageSource = "Application"; break;
        case GL_DEBUG_SOURCE_OTHER:           messageSource = "Other"; break;
    }
    const char* messageType;
    switch (type) {
        case GL_DEBUG_TYPE_ERROR:               messageType = "Error"; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: messageType = "Deprecated Behaviour"; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  messageType = "Undefined Behaviour"; break;
        case GL_DEBUG_TYPE_PORTABILITY:         messageType = "Portability"; break;
        case GL_DEBUG_TYPE_PERFORMANCE:         messageType = "Performance"; break;
        case GL_DEBUG_TYPE_MARKER:              messageType = "Marker"; break;
        case GL_DEBUG_TYPE_PUSH_GROUP:          messageType = "Push Group"; break;
        case GL_DEBUG_TYPE_POP_GROUP:           messageType = "Pop Group"; break;
        case GL_DEBUG_TYPE_OTHER:               messageType = "Other"; break;
    }
    const char* messageSeverity;
    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:         messageSeverity = "high"; break;
        case GL_DEBUG_SEVERITY_MEDIUM:       messageSeverity = "medium"; break;
        case GL_DEBUG_SEVERITY_LOW:          messageSeverity = "low"; break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: messageSeverity = "notification"; break;
    }

    LOG_WARN(
        "OpenGL debug message ({0}): {1}\n\tSource: {2}\n\tType:{3}\n\tSeverity: {4}",
        id, message, messageSource, messageType, messageSeverity
    );
}
#endif // SNV_ENABLE_DEBUG


void GLBackend::Init()
{
    LOG_INFO(
        "OpengGL Info\n"
        "\tVendor: {0}\n"
        "\tRenderer: {1}\n"
        "\tVersion: {2}",
        glGetString( GL_VENDOR ), glGetString( GL_RENDERER ), glGetString( GL_VERSION )
    );

#ifdef SNV_ENABLE_DEBUG
    i32 flags;
    glGetIntegerv( GL_CONTEXT_FLAGS, &flags );
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
        glEnable( GL_DEBUG_OUTPUT );
        glEnable( GL_DEBUG_OUTPUT_SYNCHRONOUS );
        glDebugMessageCallback( openGLMessageCallback, nullptr );
        glDebugMessageControl( GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE );
    }
#endif // SNV_ENABLE_DEBUG
}


void GLBackend::EnableBlend()
{
    glEnable( GL_BLEND );
}


void GLBackend::EnableDepthTest()
{
    glEnable( GL_DEPTH_TEST );
}


void GLBackend::SetBlendFunction(GLBlendFactor source, GLBlendFactor destination)
{
    glBlendFunc(static_cast<ui32>(source), static_cast<ui32>(destination));
}

void GLBackend::SetClearColor( f32 r, f32 g, f32 b, f32 a )
{
    glClearColor( r, g, b, a );
}

void GLBackend::SetDepthFunction( GLDepthFunction depthFunction )
{
    glDepthFunc( static_cast<GLenum>(depthFunction) );
}

void GLBackend::SetViewport( i32 x, i32 y, i32 width, i32 height )
{
    // NOTE: ??
    glViewport( 0, 0, width, height );
}


void GLBackend::Clear( GLBufferBit bufferBitMask )
{
    glClear( static_cast<GLbitfield>(bufferBitMask) );
}


void GLBackend::DrawArrays(i32 count)
{
    glDrawArrays(GL_TRIANGLES, 0, count);
}
