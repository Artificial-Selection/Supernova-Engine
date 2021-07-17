#include <Renderer/OpenGL/GLBackend.hpp>
#include <Core/Log.hpp>

#include <glad/glad.h>


namespace snv
{

constexpr ui32 g_BlendFactorToGL[] = {
    GL_ONE,                // BlendFactor::One
    GL_SRC_ALPHA,          // BlendFactor::SrcAlpha
    GL_ONE_MINUS_SRC_ALPHA // BlendFactor::OneMinusSrcAlpha
};

constexpr ui32 g_DepthFunctionToGL[] = {
    GL_NEVER,    // DepthFunction::Never
    GL_LESS,     // DepthFunction::Less
    GL_EQUAL,    // DepthFunction::Equal
    GL_LEQUAL,   // DepthFunction::LessOrEqual
    GL_GREATER,  // DepthFunction::Greater
    GL_NOTEQUAL, // DepthFunction::NotEqual
    GL_GEQUAL,   // DepthFunction::GreaterOrEqual
    GL_ALWAYS    // DepthFunction::Always
};

constexpr ui32 g_BufferBitToGL[] = {
    GL_COLOR_BUFFER_BIT,  // BufferBit::Color
    GL_DEPTH_BUFFER_BIT,  // BufferBit::Depth
    // GL_ACCUM_BUFFER_BIT,  // BufferBit::Accum
    GL_STENCIL_BUFFER_BIT // BufferBit::Stencil
};


#ifdef SNV_ENABLE_DEBUG
void APIENTRY openGLMessageCallback(
    GLenum source, GLenum type, ui32 id, GLenum severity, GLsizei length, const char* message, const void* userParam
)
{
    // Ignore OpenGL debug message (131185)
    //  "Buffer detailed info : Buffer object 786 (bound to GL_ELEMENT_ARRAY_BUFFER_ARB, usage hint is GL_STATIC_DRAW)
    //   will use VIDEO memory as the source for buffer object operations."
    if (id == 131185)
    {
        return;
    }

    const char* messageSource;
    switch(source)
    {
        case GL_DEBUG_SOURCE_API:             messageSource = "API"; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   messageSource = "Window System"; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: messageSource = "Shader Compiler"; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:     messageSource = "Third Party"; break;
        case GL_DEBUG_SOURCE_APPLICATION:     messageSource = "Application"; break;
        case GL_DEBUG_SOURCE_OTHER:           messageSource = "Other"; break;
    }
    const char* messageType;
    switch(type)
    {
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
    switch(severity)
    {
        case GL_DEBUG_SEVERITY_HIGH:
            LOG_CRITICAL(
                "OpenGL debug message ({0})\n\t{1}\n\tSource: {2}\n\tType: {3}\n\tSeverity: high",
                id, message, messageSource, messageType
            );
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            LOG_ERROR(
                "OpenGL debug message ({0})\n\t{1}\n\tSource: {2}\n\tType: {3}\n\tSeverity: medium",
                id, message, messageSource, messageType
            );
            break;
        case GL_DEBUG_SEVERITY_LOW:
            LOG_WARN(
                "OpenGL debug message ({0})\n\t{1}\n\tSource: {2}\n\tType: {3}\n\tSeverity: low",
                id, message, messageSource, messageType
            );
            break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            LOG_INFO(
                "OpenGL debug message ({0})\n\t{1}\n\tSource: {2}\n\tType: {3}\n\tSeverity: notification",
                id, message, messageSource, messageType
            );
            break;
    }
}
#endif // SNV_ENABLE_DEBUG


GLBackend::GLBackend()
{
    LOG_INFO(
        "OpengGL Info\n"
        "\tVendor: {0}\n"
        "\tRenderer: {1}\n"
        "\tVersion: {2}",
        glGetString(GL_VENDOR), glGetString(GL_RENDERER), glGetString(GL_VERSION)
    );

#ifdef SNV_ENABLE_DEBUG
    i32 flags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
    {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(openGLMessageCallback, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
    }
#endif // SNV_ENABLE_DEBUG
}


void GLBackend::EnableBlend()
{
    glEnable(GL_BLEND);
}

void GLBackend::EnableDepthTest()
{
    glEnable(GL_DEPTH_TEST);
}


void GLBackend::SetBlendFunction(BlendFactor source, BlendFactor destination)
{
    const auto sourceFactor      = g_BlendFactorToGL[static_cast<ui32>(source)];
    const auto destinationFactor = g_BlendFactorToGL[static_cast<ui32>(destination)];
    glBlendFunc(sourceFactor, destinationFactor);
}

void GLBackend::SetClearColor(f32 r, f32 g, f32 b, f32 a)
{
    glClearColor(r, g, b, a);
}

void GLBackend::SetDepthFunction(DepthFunction depthFunction)
{
    const auto function = g_DepthFunctionToGL[static_cast<ui32>(depthFunction)];
    glDepthFunc(function);
}

void GLBackend::SetViewport(i32 x, i32 y, i32 width, i32 height)
{
    // NOTE: ??
    glViewport(0, 0, width, height);
}


void GLBackend::Clear(BufferBit bufferBitMask)
{
    // TODO: Somehow make this better ???
    ui32 mask = 0;
    if (bufferBitMask & BufferBit::Color)
    {
        mask |= g_BufferBitToGL[0];
    }
    if (bufferBitMask & BufferBit::Depth)
    {
        mask |= g_BufferBitToGL[1];
    }
    if (bufferBitMask & BufferBit::Stencil)
    {
        mask |= g_BufferBitToGL[2];
    }
    glClear(mask);
}


void GLBackend::DrawGraphicsBuffer(GraphicsBufferHandle handle, i32 indexCount, i32 vertexCount)
{
    const auto& graphicsBuffer = m_graphicsBuffers[handle];
    graphicsBuffer.Bind();
    glDrawElements(GL_TRIANGLES, indexCount , GL_UNSIGNED_INT, 0);
}

void GLBackend::DrawArrays(i32 count)
{
    glDrawArrays(GL_TRIANGLES, 0, count);
}

void GLBackend::DrawElements(i32 count)
{
    LOG_ERROR("GLBackend::DrawElements() is not implemented");
    //glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, )
}


GraphicsBufferHandle GLBackend::CreateGraphicsBuffer(
    std::span<const std::byte> indexData,
    std::span<const std::byte> vertexData,
    const std::vector<VertexAttributeDescriptor>& vertexLayout
)
{
    GLGraphicsBuffer glGraphicsBuffer(indexData, vertexData, vertexLayout);
    const auto handle = glGraphicsBuffer.GetHandle();
    m_graphicsBuffers.emplace(handle, std::move(glGraphicsBuffer));

    return handle;
}

} // namespace snv
