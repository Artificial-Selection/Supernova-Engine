#include <Engine/Renderer/OpenGL/GLBackend.hpp>
#include <Engine/Renderer/OpenGL/GLImGuiRenderContext.hpp>

#include <Engine/Application/Window.hpp>
#include <Engine/Core/Log.hpp>

#include <glad/glad.h>


// TODO(v.matushkin):
//  - <ContextCreation>
//    The fact that GLFW is managing OpenGL context gives me a lot of headache
//    - <SwapBuffers>
//  - <GLFramebuffer>
//    - What to do with the default screen framebuffer?


const ui32 gl_BlendMode[] = {
    GL_ZERO,
    GL_ONE,
    GL_DST_COLOR,
    GL_SRC_COLOR,
    GL_ONE_MINUS_DST_COLOR,
    GL_SRC_ALPHA,
    GL_ONE_MINUS_SRC_COLOR,
    GL_DST_ALPHA,
    GL_ONE_MINUS_DST_ALPHA,
    GL_SRC_ALPHA_SATURATE,
    GL_ONE_MINUS_SRC_ALPHA,
};

const ui32 gl_BufferBit[] = {
    GL_COLOR_BUFFER_BIT, // BufferBit::Color
    GL_DEPTH_BUFFER_BIT, // BufferBit::Depth
    // GL_ACCUM_BUFFER_BIT,  // BufferBit::Accum
    GL_STENCIL_BUFFER_BIT // BufferBit::Stencil
};

const ui32 gl_DepthFunction[] = {
    GL_NEVER,    // DepthFunction::Never
    GL_LESS,     // DepthFunction::Less
    GL_EQUAL,    // DepthFunction::Equal
    GL_LEQUAL,   // DepthFunction::LessEqual
    GL_GREATER,  // DepthFunction::Greater
    GL_NOTEQUAL, // DepthFunction::NotEqual
    GL_GEQUAL,   // DepthFunction::GreaterEqual
    GL_ALWAYS    // DepthFunction::Always
};

// NOTE(v.matushkin): GL_NONE will be never used in gl_DepthStencilAttachment and gl_DepthStencilType
//  it serves just as a padding for FramebufferDepthStencilType::None
const ui32 gl_DepthStencilAttachment[] = {
    GL_NONE,
    GL_DEPTH_ATTACHMENT,
    GL_STENCIL_ATTACHMENT,
    GL_DEPTH_STENCIL_ATTACHMENT
};

const ui32 gl_DepthStencilType[] = {
    GL_NONE,
    GL_DEPTH,
    GL_STENCIL,
    GL_DEPTH_STENCIL
};

const ui32 gl_RenderTextureFormat[] = {
    GL_RGBA8, // TODO(v.matushkin): The fuck am I supposed to do with this RGBA/BGRA shit, I cannot specify BGRA in OpenGL
    GL_DEPTH_COMPONENT32F
};

// TODO(v.matushkin): Remove GetHandle() methods from GLShader/GLTexture/GLBuffer classes and use this
// static ui32 g_BufferHandleWorkaround        = 0;
static ui32 g_FramebufferHandleWorkaround   = 0;
static ui32 g_RenderTextureHandleWorkaround = 0;
// static ui32 g_ShaderHandleWorkaround        = 0;
// static ui32 g_TextureHandleWorkaround       = 0;


namespace snv
{

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

    // TODO(v.matushkin) Pass this settings in some struct, instead of just hardcoding it
    //  So different backends will give the same result
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // TODO(v.matushkin): <GLFramebuffer>
    GLRenderTexture colorAttachment = {
        .ID         = static_cast<ui32>(RenderTextureHandle::InvalidHandle),
        .ClearValue = {.Color = {0.f, 0.f, 0.f, 0.f}},
        .LoadAction = RenderTextureLoadAction::Clear,
    };

    m_swapchainFramebufferHandle                 = static_cast<FramebufferHandle>(g_FramebufferHandleWorkaround++);
    m_framebuffers[m_swapchainFramebufferHandle] = GLFramebuffer{
        .ID               = 0,
        .ColorAttachments = {colorAttachment},
        .DepthStencilType = GL_NONE,
    };
}


void GLBackend::EnableBlend()
{
    glEnable(GL_BLEND);
}

void GLBackend::EnableDepthTest()
{
    glEnable(GL_DEPTH_TEST);
}


void* GLBackend::GetNativeRenderTexture(RenderTextureHandle renderTextureHandle)
{
    return reinterpret_cast<void*>(m_renderTextures[renderTextureHandle].ID);
}


void GLBackend::SetBlendFunction(BlendMode source, BlendMode destination)
{
    const auto sourceMode      = gl_BlendMode[static_cast<ui8>(source)];
    const auto destinationMode = gl_BlendMode[static_cast<ui8>(destination)];
    glBlendFunc(sourceMode, destinationMode);
}

void GLBackend::SetClearColor(f32 r, f32 g, f32 b, f32 a)
{
    glClearColor(r, g, b, a);
}

void GLBackend::SetDepthFunction(DepthCompareFunction depthCompareFunction)
{
    const auto function = gl_DepthFunction[static_cast<ui8>(depthCompareFunction)];
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
        mask |= gl_BufferBit[0];
    }
    if (bufferBitMask & BufferBit::Depth)
    {
        mask |= gl_BufferBit[1];
    }
    if (bufferBitMask & BufferBit::Stencil)
    {
        mask |= gl_BufferBit[2];
    }
    glClear(mask);
}


void GLBackend::BeginFrame(const glm::mat4x4& localToWorld, const glm::mat4x4& cameraView, const glm::mat4x4& cameraProjection)
{
    // TODO(v.matushkin): Shouldn't get shader like this, tmp workaround
    // const auto& shader = m_shaders[shaderHandle];
    const auto& shader = m_shaders.begin()->second;
    shader.SetInt1("_DiffuseTexture", 0);
    shader.SetMatrix4("_ObjectToWorld", localToWorld);
    shader.SetMatrix4("_MatrixV", cameraView);
    shader.SetMatrix4("_MatrixP", cameraProjection);
    shader.Bind();
}

void GLBackend::BeginRenderPass(FramebufferHandle framebufferHandle)
{
    const auto& framebuffer = m_framebuffers[framebufferHandle];

    const auto glFramebufferID = framebuffer.ID;
    glBindFramebuffer(GL_FRAMEBUFFER, glFramebufferID);

    //- Clear Color attachments
    const auto& colorAttachments = framebuffer.ColorAttachments;

    for (ui32 i = 0; i < colorAttachments.size(); ++i)
    {
        const auto& colorAttachment = colorAttachments[i];
        if (colorAttachment.LoadAction == RenderTextureLoadAction::Clear)
        {
            // NOTE(v.matushkin): Why the fuck it doesn't work with GL_DRAW_BUFFER0 + i ?
            glClearNamedFramebufferfv(glFramebufferID, GL_COLOR, i, colorAttachment.ClearValue.Color);
        }
    }

    //- Clear DepthStencil attachment
    // NOTE(v.matushkin): Is it an UB if I get the address of ClearValue.DepthStencil when depthStencilType == GL_NONE ?
    const auto& depthStencilAttachment = framebuffer.DepthStencilAttachment;
    const auto& depthStencilClearValue = depthStencilAttachment.ClearValue.DepthStencil;
    const auto  depthStencilType       = framebuffer.DepthStencilType;

    if (depthStencilType != GL_NONE && depthStencilAttachment.LoadAction == RenderTextureLoadAction::Clear)
    {
        if (depthStencilType == GL_DEPTH)
        {
            glClearNamedFramebufferfv(glFramebufferID, GL_DEPTH, 0, &depthStencilClearValue.Depth);
        }
        else if (depthStencilType == GL_STENCIL)
        {
            i32 stencilClearValue = depthStencilClearValue.Stencil;
            glClearNamedFramebufferiv(glFramebufferID, GL_STENCIL, 0, &stencilClearValue);
        }
        else if (depthStencilType == GL_DEPTH_STENCIL)
        {
            glClearNamedFramebufferfi(glFramebufferID, GL_DEPTH_STENCIL, 0, depthStencilClearValue.Depth, depthStencilClearValue.Stencil);
        }
    }
}

void GLBackend::BeginRenderPass(FramebufferHandle framebufferHandle, RenderTextureHandle input)
{
    BeginRenderPass(framebufferHandle);
}

void GLBackend::EndFrame()
{
    // TODO(v.matushkin): <ContextCreation/SwapBuffers>
    Window::SwapBuffers();
}


void GLBackend::DrawBuffer(TextureHandle textureHandle, BufferHandle bufferHandle, i32 indexCount, i32 vertexCount)
{
    const auto& texture        = m_textures[textureHandle];
    const auto& graphicsBuffer = m_buffers[bufferHandle];

    texture.Bind(0);
    graphicsBuffer.Bind();

    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
}


IImGuiRenderContext* GLBackend::CreateImGuiRenderContext()
{
    // NOTE(v.matushkin): I don't know if render backend should manage imgui context lifetime
    return new GLImGuiRenderContext();
}


GraphicsState GLBackend::CreateGraphicsState(const GraphicsStateDesc& graphicsStateDesc)
{
    //- Create Framebuffer
    GLFramebuffer framebuffer;
    glCreateFramebuffers(1, &framebuffer.ID);

    GraphicsState graphicsState;

    //- Create Color Attachments
    const auto& colorAttachmentsDesc = graphicsStateDesc.ColorAttachments;

    for (ui32 i = 0; i < colorAttachmentsDesc.size(); ++i)
    {
        const auto renderTextureDesc     = colorAttachmentsDesc[i];
        //-- Create RenderTexture
        const auto glRenderTextureFormat = gl_RenderTextureFormat[static_cast<ui32>(renderTextureDesc.Format)];

        ui32 glRenderTextureID;
        glCreateTextures(GL_TEXTURE_2D, 1, &glRenderTextureID);
        glTextureStorage2D(glRenderTextureID, 1, glRenderTextureFormat, renderTextureDesc.Width, renderTextureDesc.Height);

        GLRenderTexture glRenderTexture = {
            .ID         = glRenderTextureID,
            .ClearValue = renderTextureDesc.ClearValue,
            .LoadAction = renderTextureDesc.LoadAction,
        };

        //-- Bind RenderTexture to Framebuffer
        glNamedFramebufferTexture(framebuffer.ID, GL_COLOR_ATTACHMENT0 + i, glRenderTexture.ID, 0);

        const auto renderTextureHandle        = static_cast<RenderTextureHandle>(g_RenderTextureHandleWorkaround++);
        m_renderTextures[renderTextureHandle] = glRenderTexture;
        graphicsState.ColorAttachments.push_back(renderTextureHandle);

        framebuffer.ColorAttachments.push_back(glRenderTexture);
    }

    //- Create Depth Stencil Attachment
    const auto& depthStencilAttachmentDesc = graphicsStateDesc.DepthStencilAttachment;
    const auto  descDepthStencilType       = graphicsStateDesc.DepthStencilType;

    if (descDepthStencilType != FramebufferDepthStencilType::None)
    {
        auto& depthStenscilAttachment = framebuffer.DepthStencilAttachment;
    
        depthStenscilAttachment.ClearValue = depthStencilAttachmentDesc.ClearValue;
        depthStenscilAttachment.LoadAction = depthStencilAttachmentDesc.LoadAction;
    
        const auto glRenderTextureFormat    = gl_RenderTextureFormat[static_cast<ui8>(depthStencilAttachmentDesc.Format)];
        const auto glDepthStencilAttachment = gl_DepthStencilAttachment[static_cast<ui8>(descDepthStencilType)];
        const auto glDepthStencilType       = gl_DepthStencilType[static_cast<ui8>(descDepthStencilType)];
    
        framebuffer.DepthStencilType = glDepthStencilType;
    
        //-- Create RenderBuffer
        glCreateRenderbuffers(1, &depthStenscilAttachment.ID);
        glNamedRenderbufferStorage(
            depthStenscilAttachment.ID,
            glRenderTextureFormat,
            depthStencilAttachmentDesc.Width,
            depthStencilAttachmentDesc.Height
        );
    
        //-- Bind RenderBuffer to Framebuffer
        glNamedFramebufferRenderbuffer(framebuffer.ID, glDepthStencilAttachment, GL_RENDERBUFFER, depthStenscilAttachment.ID);
    
        const auto renderTextureHandle        = static_cast<RenderTextureHandle>(g_RenderTextureHandleWorkaround++);
        m_renderTextures[renderTextureHandle] = depthStenscilAttachment;
        graphicsState.DepthStencilAttachment  = renderTextureHandle;
    }
    else
    {
        framebuffer.DepthStencilType = GL_NONE;
    }

    const auto framebufferHandle      = static_cast<FramebufferHandle>(g_FramebufferHandleWorkaround++);
    graphicsState.Framebuffer         = framebufferHandle;
    m_framebuffers[framebufferHandle] = std::move(framebuffer);

    return graphicsState;
}

BufferHandle GLBackend::CreateBuffer(
    std::span<const std::byte>              indexData,
    std::span<const std::byte>              vertexData,
    const std::vector<VertexAttributeDesc>& vertexLayout
)
{
    GLBuffer   glBuffer(indexData, vertexData, vertexLayout);
    const auto handle = glBuffer.GetHandle();
    m_buffers.emplace(handle, std::move(glBuffer));

    return handle;
}

TextureHandle GLBackend::CreateTexture(const TextureDesc& textureDesc, const ui8* textureData)
{
    GLTexture  glTexture(textureDesc, textureData);
    const auto handle = glTexture.GetHandle();
    m_textures.emplace(handle, std::move(glTexture));

    return handle;
}

ShaderHandle GLBackend::CreateShader(const ShaderDesc& shaderDesc)
{
    GLShader   glShader(shaderDesc.VertexSource, shaderDesc.FragmentSource);
    const auto handle = glShader.GetHandle();
    m_shaders.emplace(handle, std::move(glShader));

    return handle;
}

} // namespace snv
