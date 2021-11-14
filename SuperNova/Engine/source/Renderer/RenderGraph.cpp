#include <Engine/Renderer/RenderGraph.hpp>
#include <Engine/Renderer/RenderContext.hpp>
#include <Engine/Renderer/Renderer.hpp>

#include <utility>


// TODO(v.matushkin):
//  - <AttachmentNames>
//    Right now the connection between RenderTexture and its name done in a shitty way, but idk how to improve it


namespace snv
{

RenderGraph::RenderGraph(std::vector<IRenderPass*>&& renderPasses)
    : m_renderPasses(std::move(renderPasses))
{
    for (auto* renderPass : m_renderPasses)
    {
        renderPass->OnCreate(*this);
    }
}


void RenderGraph::Execute(const RenderContext& renderContext) const
{
    for (const auto* renderPass : m_renderPasses)
    {
        renderPass->OnRender(renderContext);
    }
}


void* RenderGraph::GetNativeRenderTexture(const std::string& name) const
{
    return Renderer::GetNativeRenderTexture(m_renderTextures.at(name));
}

FramebufferHandle RenderGraph::GetSwapchainFramebuffer() const
{
    return Renderer::GetSwapchainFramebuffer();
}


GraphicsState RenderGraph::CreateGraphicsState(
    const GraphicsStateDesc&        graphicsStateDesc,
    const std::vector<std::string>& attachmentNames)
{
    auto graphicsState = Renderer::CreateGraphicsState(graphicsStateDesc);
    // Color
    for (ui32 i = 0; i < attachmentNames.size() - 1; ++i)
    {
        m_renderTextures[attachmentNames[i]] = graphicsState.ColorAttachments[i];
    }
    // Depth Stencil
    if (graphicsStateDesc.DepthStencilType != FramebufferDepthStencilType::None)
    {
        // TODO(v.matushkin): <AttachmentNames>
        m_renderTextures[*(attachmentNames.end() - 1)] = graphicsState.DepthStencilAttachment;
    }

    return graphicsState;
}

} // namespace snv
