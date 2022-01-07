#include <Engine/Renderer/RenderGraph.hpp>
#include <Engine/Renderer/RenderContext.hpp>
#include <Engine/Renderer/Renderer.hpp>

#include <utility>


// TODO(v.matushkin):
//  - <AttachmentNames>
//    Right now the connection between RenderTexture and its name done in a shitty way, but idk how to improve it
//
//  - <GetNativeRenderTexture>
//    This method probably can be removed when I'll replace '#include <imgui_impl_*.h>' with my own implementation


namespace snv
{

// ----------------------------------------------------------------------------------------------------
// ------------------------------------------- RenderGraph --------------------------------------------
// ----------------------------------------------------------------------------------------------------

void RenderGraph::Build(const std::string& outputRenderTextureName)
{
    std::stack<RenderPassID> renderPassStack;
    m_renderTextureUsages.clear();

    RenderTextureID textureToRead = m_renderTextureNameToID[outputRenderTextureName];
    while (textureToRead != RenderTextureID::InvalidID)
    {
        const auto& rtAccess = m_renderTexturesAccess[static_cast<ui8>(textureToRead)];
        renderPassStack.push(rtAccess.CreatePassID);

        const auto& renderGraphNode   = m_renderGraphNodes[static_cast<ui8>(rtAccess.CreatePassID)];
        const auto  readAttachmentID  = renderGraphNode.ReadAttachmentID;
        const auto  hasReadAttachment = readAttachmentID != RenderTextureID::InvalidID;

        for (const auto renderTextureID : renderGraphNode.CreateAttachmentsID)
        {
            m_renderTextureUsages[renderTextureID].push(AttachmentUsage::Create);
        }
        if (hasReadAttachment)
        {
            m_renderTextureUsages[readAttachmentID].push(AttachmentUsage::Read);
            textureToRead = readAttachmentID;
        }
        else
        {
            textureToRead = RenderTextureID::InvalidID;
        }
    }

    m_renderPasses.clear();

    while (renderPassStack.empty() == false)
    {
        const auto& renderGraphNode = m_renderGraphNodes[static_cast<ui8>(renderPassStack.top())];
        m_renderPasses.push_back(renderGraphNode.Pass.get());

        renderPassStack.pop();
    }

    for (auto* renderPass : m_renderPasses)
    {
        auto renderPassBuilder = RenderPassBuilder(*this);
        renderPass->OnCreate(renderPassBuilder);
    }
}

void RenderGraph::Execute(const RenderContext& renderContext) const
{
    for (const auto* renderPass : m_renderPasses)
    {
        renderPass->OnRender(renderContext);
    }
}


// ----------------------------------------------------------------------------------------------------
// --------------------------------------- RenderPassScheduler ----------------------------------------
// ----------------------------------------------------------------------------------------------------

RenderPassScheduler::RenderPassScheduler(RenderPassID renderPassID, RenderGraphNode& node, RenderGraph& renderGraph)
    : m_renderPassID(renderPassID)
    , m_renderTextureNameToID(renderGraph.m_renderTextureNameToID)
    , m_renderGraphNode(node)
    , m_renderTexturesAccess(renderGraph.m_renderTexturesAccess)
{}


void RenderPassScheduler::CreateTexture(const std::string& renderTextureName)
{
    RenderTextureID renderTextureID = GetRenderTextureID(renderTextureName);

    auto& renderTextureAccess = m_renderTexturesAccess[static_cast<ui8>(renderTextureID)];
    SNV_ASSERT(renderTextureAccess.CreatePassID == RenderPassID::InvalidID, "Trying to create RenderTexture with the same name twice");
    renderTextureAccess.CreatePassID = m_renderPassID;

    m_renderGraphNode.CreateAttachmentsID.push_back(renderTextureID);
}

void RenderPassScheduler::ReadTexture(const std::string& renderTextureName)
{
    RenderTextureID renderTextureID = GetRenderTextureID(renderTextureName);

    auto& renderTextureAccess = m_renderTexturesAccess[static_cast<ui8>(renderTextureID)];
    SNV_ASSERT(m_renderGraphNode.ReadAttachmentID == RenderTextureID::InvalidID, "Only one RenderTexture read access is supported");
    renderTextureAccess.ReadPassID = m_renderPassID;

    SNV_ASSERT(m_renderGraphNode.ReadAttachmentID == RenderTextureID::InvalidID, "RenderPass can read only one RenderTexture");
    m_renderGraphNode.ReadAttachmentID = renderTextureID;
}


RenderPassScheduler::RenderTextureID RenderPassScheduler::GetRenderTextureID(const std::string& renderTextureName)
{
    RenderTextureID renderTextureID;
    if (const auto it = m_renderTextureNameToID.find(renderTextureName); it != m_renderTextureNameToID.end())
    {
        renderTextureID = it->second;
    }
    else
    {
        renderTextureID = static_cast<RenderTextureID>(m_renderTexturesAccess.size());
        m_renderTexturesAccess.push_back(RenderTextureAccess{
            .CreatePassID = RenderPassID::InvalidID,
            .ReadPassID   = RenderPassID::InvalidID,
        });
        m_renderTextureNameToID[renderTextureName] = renderTextureID;
    }

    return renderTextureID;
}


// ----------------------------------------------------------------------------------------------------
// ---------------------------------------- RenderPassBuilder -----------------------------------------
// ----------------------------------------------------------------------------------------------------

RenderPassBuilder::RenderPassBuilder(RenderGraph& renderGraph)
    : m_renderTextures(renderGraph.m_renderTextures)
    , m_renderTextureUsages(renderGraph.m_renderTextureUsages)
    , m_renderTextureNameToID(renderGraph.m_renderTextureNameToID)
    , m_renderTexturesAccess(renderGraph.m_renderTexturesAccess)
{}


RenderTextureHandle RenderPassBuilder::GetRenderTexture(const std::string& name) const
{
    return m_renderTextures.at(name);
}

void* RenderPassBuilder::GetNativeRenderTexture(RenderTextureHandle renderTextureHandle) const
{
    return Renderer::GetNativeRenderTexture(renderTextureHandle);
}

RenderPassHandle RenderPassBuilder::GetSwapchainRenderPass() const
{
    return Renderer::GetSwapchainRenderPass();
}


RenderTextureHandle RenderPassBuilder::CreateRenderTexture(
    const std::string&             name,
    ui32                           width,
    ui32                           height,
    RenderTextureFormat            format,
    const RenderTextureClearValue& clearValue
)
{
    const auto  renderTextureID     = m_renderTextureNameToID[name];
    const auto& renderTextureAccess = m_renderTexturesAccess[static_cast<ui8>(renderTextureID)];

    const auto renderTextureUsage = renderTextureAccess.ReadPassID == RenderPassID::InvalidID
                                  ? RenderTextureUsage::Default
                                  : RenderTextureUsage::ShaderRead;

    RenderTextureDesc renderTextureDesc = {
        .Name       = name,
        .ClearValue = clearValue,
        .Width      = width,
        .Height     = height,
        .Format     = format,
        .Usage      = renderTextureUsage,
    };

    const auto renderTextureHandle = Renderer::CreateRenderTexture(renderTextureDesc);
    m_renderTextures[name]         = renderTextureHandle;

    return renderTextureHandle;
}

RenderPassHandle RenderPassBuilder::CreateRenderPass(
    const std::vector<RenderTextureHandle>&& colorAttachments,
    std::optional<RenderTextureHandle>       depthStencilAttachment
)
{
    return Renderer::CreateRenderPass(RenderPassDesc{
        .ColorAttachments       = std::move(colorAttachments),
        .DepthStencilAttachment = depthStencilAttachment,
    });
}

} // namespace snv
