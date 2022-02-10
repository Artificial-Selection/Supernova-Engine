#include <Engine/Renderer/RenderPasses/EngineRenderPass.hpp>
#include <Engine/Renderer/RenderPasses/ResourceNames.hpp>
#include <Engine/Renderer/RenderContext.hpp>
#include <Engine/Renderer/RenderGraph.hpp>

#include <Engine/EngineSettings.hpp>


#include <Engine/Core/Log.hpp> // TODO(v.matushkin): Remove


#include <string>
#include <vector>


namespace snv
{

EngineRenderPass::EngineRenderPass()
    : m_name("Engine")
    , m_renderPassHandle(RenderPassHandle::InvalidHandle)
{}


void EngineRenderPass::OnSchedule(RenderPassScheduler& renderPassScheduler) const
{
    LOG_INFO("EngineRenderPass::OnSchedule");
    renderPassScheduler.CreateTexture(ResourceNames::EngineColor);
    renderPassScheduler.CreateTexture(ResourceNames::EngineDepth);
}

void EngineRenderPass::OnCreate(RenderPassBuilder& renderPassBuilder)
{
    LOG_INFO("EngineRenderPass::OnCreate");
    const auto renderWidth  = EngineSettings::GraphicsSettings.RenderWidth;
    const auto renderHeight = EngineSettings::GraphicsSettings.RenderHeight;

    const auto engineColorRenderTextureHandle = renderPassBuilder.CreateRenderTexture(
        ResourceNames::EngineColor,
        renderWidth,
        renderHeight,
        RenderTextureFormat::BGRA32,
        {.Color = {0.f, 0.f, 0.f, 0.f}}
    );
    const auto engineDepthRenderTextureHandle = renderPassBuilder.CreateRenderTexture(
        ResourceNames::EngineDepth,
        renderWidth,
        renderHeight,
        RenderTextureFormat::Depth32,
        {.DepthStencil = {.Depth = 1.f, .Stencil = 0}}
    );

    m_renderPassHandle = renderPassBuilder.CreateRenderPass(RenderPassDesc{
        .ColorAttachments = {
            {
                .RenderTextureHandle = engineColorRenderTextureHandle,
                .LoadAction          = AttachmentLoadAction::Clear,
                .StoreAction         = AttachmentStoreAction::Store,
                .InitialLayout       = AttachmentLayout::ShaderSample,
                .FinalLayout         = AttachmentLayout::ShaderSample,
            },
        },
        .DepthStencilAttachment = AttachmentDesc{
            .RenderTextureHandle = engineDepthRenderTextureHandle,
            .LoadAction          = AttachmentLoadAction::Clear,
            .StoreAction         = AttachmentStoreAction::DontCare,
            .InitialLayout       = AttachmentLayout::Render,
            .FinalLayout         = AttachmentLayout::Render,
        },
        .Subpass = {
            .ColorAttachmentIndices    = {0},
            .UseDepthStencilAttachment = true,
        },
    });

    // TODO(v.matushkin): <CreateRenderPass>
    // m_renderPassHandle = renderPassBuilder.CreateRenderPass(
    //     {engineColorRenderTextureHandle},
    //     engineDepthRenderTextureHandle,
    //     SubpassDesc{
    //         .ColorAttachmentIndices    = {0},
    //         .UseDepthStencilAttachment = true,
    //     }
    // );
}

void EngineRenderPass::OnRender(const RenderContext& renderContext) const
{
    renderContext.BeginRenderPass(m_renderPassHandle);
    renderContext.DrawRenderers();
    renderContext.EndRenderPass();
}

} // namespace snv
