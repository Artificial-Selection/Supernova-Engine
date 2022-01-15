#include <Engine/Renderer/OpenGL/GLBackend.hpp>

#include <Engine/EngineSettings.hpp>
#include <Engine/Application/Window.hpp>
#include <Engine/Core/Assert.hpp>
#include <Engine/Renderer/OpenGL/GLImGuiRenderContext.hpp>

#include <glad/glad.h>


// TODO(v.matushkin):
//  - <ContextCreation>
//    The fact that GLFW is managing OpenGL context gives me a lot of headache
//    - <SwapBuffers>
//  - <GLRenderPass>
//    - What to do with the default screen framebuffer?
//  - <glObjectLabel>
//    How the fuck am I supposed to use it? Is it even worth it


static const ui32 gl_BufferBit[] = {
    GL_COLOR_BUFFER_BIT,   // BufferBit::Color
    GL_DEPTH_BUFFER_BIT,   // BufferBit::Depth
    // GL_ACCUM_BUFFER_BIT,  // BufferBit::Accum
    GL_STENCIL_BUFFER_BIT, // BufferBit::Stencil
};

#define OPENGL_NO_DEPTH_STENCIL 0
static const ui32 gl_DepthStencilAttachment[] = {
    OPENGL_NO_DEPTH_STENCIL,
    GL_DEPTH_ATTACHMENT,
    GL_STENCIL_ATTACHMENT,
    GL_DEPTH_STENCIL_ATTACHMENT,
};

static const ui32 gl_DepthStencilType[] = {
    OPENGL_NO_DEPTH_STENCIL, // NOTE(v.matushkin): Only needed as a padding
    GL_DEPTH,
    GL_STENCIL,
    GL_DEPTH_STENCIL,
};

static const ui32 gl_RenderTextureFormat[] = {
    GL_RGBA8, // TODO(v.matushkin): The fuck am I supposed to do with this RGBA/BGRA shit, I cannot specify BGRA in OpenGL
    GL_DEPTH_COMPONENT32F,
};

// TODO(v.matushkin): Remove GetHandle() methods from GLShader/GLTexture/GLBuffer classes and use this
// static ui32 g_BufferHandleWorkaround        = 0;
static ui32 g_RenderPassHandleWorkaround    = 0;
static ui32 g_RenderTextureHandleWorkaround = 0;
static ui32 g_ShaderHandleWorkaround        = 0;
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
    switch (source)
    {
        case GL_DEBUG_SOURCE_API:             messageSource = "API"; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   messageSource = "Window System"; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: messageSource = "Shader Compiler"; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:     messageSource = "Third Party"; break;
        case GL_DEBUG_SOURCE_APPLICATION:     messageSource = "Application"; break;
        case GL_DEBUG_SOURCE_OTHER:           messageSource = "Other"; break;
    }
    const char* messageType;
    switch (type)
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
    switch (severity)
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
    : m_engineShaderHandle(ShaderHandle::InvalidHandle)
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

    //- Set default state
    {
        const auto& graphicsSettings = EngineSettings::GraphicsSettings;
        glViewport(0, 0, graphicsSettings.RenderWidth, graphicsSettings.RenderHeight);

        glDisable(GL_STENCIL_TEST);
        glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_KEEP);
        // glStencilFuncSeparate(GL_FRONT, GL_ALWAYS, 0, );
        glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_KEEP);
        // glStencilFuncSeparate(GL_BACK, GL_ALWAYS, 0, );
        // glStencilMask | glStencilMaskSeparate
    }

    // TODO(v.matushkin): <GLRenderPass>
    GLRenderTexture colorAttachment = {
        .ID         = static_cast<ui32>(RenderTextureHandle::InvalidHandle),  // NOTE(v.matushkin): And how the fuck this ID will work?
        .ClearValue = {.Color = {0.f, 0.f, 0.f, 0.f}},
        .LoadAction = RenderTextureLoadAction::Clear,
    };

    m_swapchainRenderPassHandle = static_cast<RenderPassHandle>(g_RenderPassHandleWorkaround++);
    m_renderPasses[m_swapchainRenderPassHandle] = {
        .FramebufferID    = 0,
        .ColorAttachments = {colorAttachment},
        // .DepthStencilAttachment
        .DepthStencilType = OPENGL_NO_DEPTH_STENCIL,
    };
}


void* GLBackend::GetNativeRenderTexture(RenderTextureHandle renderTextureHandle)
{
    return reinterpret_cast<void*>(m_renderTextures[renderTextureHandle].ID);
}


// void GLBackend::EnableBlend()
// {
//     glEnable(GL_BLEND);
// }
// 
// void GLBackend::EnableDepthTest()
// {
//     glEnable(GL_DEPTH_TEST);
// }
// 
// void GLBackend::SetBlendFunction(BlendMode source, BlendMode destination)
// {
//     const auto sourceMode      = gl_BlendMode[static_cast<ui8>(source)];
//     const auto destinationMode = gl_BlendMode[static_cast<ui8>(destination)];
//     glBlendFunc(sourceMode, destinationMode);
// }
// 
// void GLBackend::SetClearColor(f32 r, f32 g, f32 b, f32 a)
// {
//     glClearColor(r, g, b, a);
// }
// 
// void GLBackend::SetDepthFunction(CompareFunction depthCompareFunction)
// {
//     const auto function = gl_DepthFunction[static_cast<ui8>(depthCompareFunction)];
//     glDepthFunc(function);
// }
// 
// void GLBackend::SetViewport(i32 x, i32 y, i32 width, i32 height)
// {
//     // NOTE: ??
//     glViewport(0, 0, width, height);
// }
// 
// void GLBackend::Clear(BufferBit bufferBitMask)
// {
//     // TODO: Somehow make this better ???
//     ui32 mask = 0;
//     if (bufferBitMask & BufferBit::Color)
//     {
//         mask |= gl_BufferBit[0];
//     }
//     if (bufferBitMask & BufferBit::Depth)
//     {
//         mask |= gl_BufferBit[1];
//     }
//     if (bufferBitMask & BufferBit::Stencil)
//     {
//         mask |= gl_BufferBit[2];
//     }
//     glClear(mask);
// }


void GLBackend::BeginFrame(const glm::mat4x4& localToWorld, const glm::mat4x4& cameraView, const glm::mat4x4& cameraProjection)
{
    // TODO(v.matushkin): Shouldn't get shader like this, tmp workaround
    // const auto& shader = m_shaders[shaderHandle];
    const auto& shader = m_shaders[m_engineShaderHandle];
    shader.SetInt1("_DiffuseTexture", 0);
    shader.SetMatrix4("_ObjectToWorld", localToWorld);
    shader.SetMatrix4("_MatrixV", cameraView);
    shader.SetMatrix4("_MatrixP", cameraProjection);
}

void GLBackend::BeginRenderPass(RenderPassHandle renderPassHandle)
{
    const auto& glRenderPass = m_renderPasses[renderPassHandle];

    const auto glFramebufferID = glRenderPass.FramebufferID;
    glBindFramebuffer(GL_FRAMEBUFFER, glFramebufferID);

    //- Clear Color attachments
    const auto& colorAttachments = glRenderPass.ColorAttachments;

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
    const auto depthStencilType = glRenderPass.DepthStencilType;

    if (depthStencilType != OPENGL_NO_DEPTH_STENCIL)
    {
        const auto& depthStencilAttachment = glRenderPass.DepthStencilAttachment;

        if (depthStencilAttachment.LoadAction == RenderTextureLoadAction::Clear)
        {
            const auto depthStencilClearValue = depthStencilAttachment.ClearValue.DepthStencil;

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
            else
            {
                SNV_ASSERT(false, "DepthStencil GLRenderTexture::Type holds an invalid value");
            }
        }
    }
}

void GLBackend::BeginRenderPass(RenderPassHandle renderPassHandle, RenderTextureHandle input)
{
    BeginRenderPass(renderPassHandle);
}

void GLBackend::EndFrame()
{
    // TODO(v.matushkin): <ContextCreation/SwapBuffers>
    Window::SwapBuffers();
}


void GLBackend::BindShader(ShaderHandle shaderHandle)
{
    const auto& openglShader = m_shaders[shaderHandle];
    openglShader.Bind();
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


RenderTextureHandle GLBackend::CreateRenderTexture(const RenderTextureDesc& renderTextureDesc)
{
    const auto glRenderTextureFormat = gl_RenderTextureFormat[static_cast<ui8>(renderTextureDesc.Format)];

    bool isRenderbuffer = renderTextureDesc.Usage == RenderTextureUsage::Default;
    ui32 glRenderTextureID;
    if (isRenderbuffer)
    {
        glCreateRenderbuffers(1, &glRenderTextureID);
        glNamedRenderbufferStorage(glRenderTextureID, glRenderTextureFormat, renderTextureDesc.Width,
        renderTextureDesc.Height);
    }
    else
    {
        glCreateTextures(GL_TEXTURE_2D, 1, &glRenderTextureID);
        glTextureStorage2D(glRenderTextureID, 1, glRenderTextureFormat, renderTextureDesc.Width, renderTextureDesc.Height);
    }

    GLRenderTexture glRenderTexture = {
        .ID                         = glRenderTextureID,
        .DepthStencilAttachmentType = gl_DepthStencilAttachment[static_cast<ui8>(renderTextureDesc.RenderTextureType())],
        .ClearValue                 = renderTextureDesc.ClearValue,
        .LoadAction                 = renderTextureDesc.LoadAction,
        .IsRenderbuffer             = isRenderbuffer,
    };

    const auto renderTextureHandle        = static_cast<RenderTextureHandle>(g_RenderTextureHandleWorkaround++);
    m_renderTextures[renderTextureHandle] = glRenderTexture;

    return renderTextureHandle;
}

RenderPassHandle GLBackend::CreateRenderPass(const RenderPassDesc& renderPassDesc)
{
    GLRenderPass glRenderPass;
    glCreateFramebuffers(1, &glRenderPass.FramebufferID);

    //- Bind Color attachment to Framebuffer
    const auto& colorAttachmentHandles = renderPassDesc.ColorAttachments;

    for (ui32 i = 0; i < colorAttachmentHandles.size(); ++i)
    {
        const auto& glRenderTexture = m_renderTextures[colorAttachmentHandles[i]];
    
        if (glRenderTexture.IsRenderbuffer)
        {
            glNamedFramebufferRenderbuffer(glRenderPass.FramebufferID, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, glRenderTexture.ID);
        }
        else
        {
            glNamedFramebufferTexture(glRenderPass.FramebufferID, GL_COLOR_ATTACHMENT0, glRenderTexture.ID, 0);
        }
    
        glRenderPass.ColorAttachments.push_back(glRenderTexture);
    }

    //- Bind DepthStencil attachment to Framebuffer
    const auto& depthStencilAttachmentHandle = renderPassDesc.DepthStencilAttachment;

    if (depthStencilAttachmentHandle.has_value())
    {
        const auto& glRenderTexture = m_renderTextures[depthStencilAttachmentHandle.value()];
        const auto  depthStencilAttachmentType = glRenderTexture.DepthStencilAttachmentType;

        if (glRenderTexture.IsRenderbuffer)
        {
            glNamedFramebufferRenderbuffer(glRenderPass.FramebufferID, depthStencilAttachmentType, GL_RENDERBUFFER, glRenderTexture.ID);
        }
        else
        {
            glNamedFramebufferTexture(glRenderPass.FramebufferID, depthStencilAttachmentType, glRenderTexture.ID, 0);
        }

        glRenderPass.DepthStencilAttachment = glRenderTexture;
        switch (depthStencilAttachmentType)
        {
            case GL_DEPTH_ATTACHMENT:         glRenderPass.DepthStencilType = GL_DEPTH;         break;
            case GL_STENCIL_ATTACHMENT:       glRenderPass.DepthStencilType = GL_STENCIL;       break;
            case GL_DEPTH_STENCIL_ATTACHMENT: glRenderPass.DepthStencilType = GL_DEPTH_STENCIL; break;
        }
    }
    else
    {
        glRenderPass.DepthStencilType = OPENGL_NO_DEPTH_STENCIL;
    }

    const auto renderPassHandle      = static_cast<RenderPassHandle>(g_RenderPassHandleWorkaround++);
    m_renderPasses[renderPassHandle] = std::move(glRenderPass);

    return renderPassHandle;
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
    const auto shaderHandle = static_cast<ShaderHandle>(g_ShaderHandleWorkaround++);
    m_shaders.emplace(shaderHandle, GLShader(shaderDesc));

    if (shaderDesc.Name == "Engine/Main")
    {
        m_engineShaderHandle = shaderHandle;
    }

    return shaderHandle;
}

} // namespace snv


#undef OPENGL_NO_DEPTH_STENCIL
