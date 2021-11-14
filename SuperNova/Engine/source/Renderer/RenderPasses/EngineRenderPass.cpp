#include <Engine/Renderer/RenderPasses/EngineRenderPass.hpp>
#include <Engine/Renderer/RenderPasses/ResourceNames.hpp>
#include <Engine/Renderer/RenderContext.hpp>
#include <Engine/Renderer/RenderGraph.hpp>

#include <Engine/EngineSettings.hpp>

#include <string>
#include <vector>


namespace snv
{

EngineRenderPass::EngineRenderPass()
    : m_framebufferHandle(FramebufferHandle::InvalidHandle)
{}


void EngineRenderPass::OnCreate(RenderGraph& renderGraph)
{
    const auto renderWidth  = EngineSettings::GraphicsSettings.RenderWidth;
    const auto renderHeight = EngineSettings::GraphicsSettings.RenderHeight;

    GraphicsStateDesc graphicsStateDesc = {
        .ColorAttachments = {
            {
                .ClearValue = {.Color = {0.f, 0.f, 0.f, 0.f}},
                .Width      = renderWidth,
                .Height     = renderHeight,
                .Format     = RenderTextureFormat::BGRA32,
                .LoadAction = RenderTextureLoadAction::Clear,
                .Usage      = RenderTextureUsage::ShaderRead, // TODO(v.matushkin): Hardcoded, RenderGraph should set this
            },
        },
        .DepthStencilAttachment = {
                .ClearValue = {.DepthStencil = {.Depth = 1.f, .Stencil = 0}},
                .Width      = renderWidth,
                .Height     = renderHeight,
                .Format     = RenderTextureFormat::Depth32,
                .LoadAction = RenderTextureLoadAction::Clear,
                .Usage      = RenderTextureUsage::Default
        },
        .DepthStencilType = FramebufferDepthStencilType::Depth,
    };

    std::vector<std::string> attachmentNames = {ResourceNames::EngineColor, ResourceNames::EngineDepth};

    auto graphicsState = renderGraph.CreateGraphicsState(graphicsStateDesc, attachmentNames);

    m_framebufferHandle = graphicsState.Framebuffer;
}

void EngineRenderPass::OnRender(const RenderContext& renderContext) const
{
    renderContext.BeginRenderPass(m_framebufferHandle);
    renderContext.DrawRenderers();
}

} // namespace snv
