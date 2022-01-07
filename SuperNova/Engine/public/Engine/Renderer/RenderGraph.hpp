#pragma once

// NOTE(v.matushkin): Remove Assert.hpp include when I rework RenderGraph::AddRenderPass() to take already created IRenderPass instance.
//  And add something like IRenderPass::OnSchedule() method.
#include <Engine/Core/Assert.hpp>
#include <Engine/Renderer/IRenderPass.hpp>
#include <Engine/Renderer/RenderTypes.hpp>

#include <memory>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>


namespace snv
{

class RenderContext;


class RenderGraph
{
    friend class RenderPassBuilder;
    friend class RenderPassScheduler;


    enum class AttachmentUsage : ui8
    {
        Create,
        Read,
    };

    enum class RenderPassID    : ui8 { InvalidID = 255 };
    enum class RenderTextureID : ui8 { InvalidID = 255 };

    struct RenderGraphNode
    {
        std::unique_ptr<IRenderPass> Pass;
        std::vector<RenderTextureID> CreateAttachmentsID;
        RenderTextureID              ReadAttachmentID;
    };

    struct RenderTextureAccess
    {
        RenderPassID CreatePassID;
        RenderPassID ReadPassID;
    };


    using RenderTextureUsageMap = std::unordered_map<RenderTextureID, std::stack<AttachmentUsage>>;
    using RenderPassIDMap       = std::unordered_map<std::string, RenderPassID>;
    using RenderTextureIDMap    = std::unordered_map<std::string, RenderTextureID>;

public:
    RenderGraph() = default;
    ~RenderGraph() = default;

    void Build(const std::string& outputRenderTextureName);
    void Execute(const RenderContext& renderContext) const;

    template<CRenderPass T>
    void AddRenderPass(std::string&& renderPassName);

private:
    // Created resources
    std::unordered_map<std::string, RenderTextureHandle> m_renderTextures;

    // Current render passes
    std::vector<IRenderPass*> m_renderPasses;
    RenderTextureUsageMap     m_renderTextureUsages;

    // Render passes schedule info
    RenderPassIDMap    m_renderPassNameToID;
    RenderTextureIDMap m_renderTextureNameToID;

    std::vector<RenderGraphNode>     m_renderGraphNodes;
    std::vector<RenderTextureAccess> m_renderTexturesAccess;
};


class RenderPassScheduler
{
    friend RenderGraph;

    using RenderGraphNode     = RenderGraph::RenderGraphNode;
    using RenderTextureAccess = RenderGraph::RenderTextureAccess;
    using RenderPassID        = RenderGraph::RenderPassID;
    using RenderTextureID     = RenderGraph::RenderTextureID;
    using RenderTextureIDMap  = RenderGraph::RenderTextureIDMap;

public:
    // NOTE(v.matushkin): Return RenderTextureID so there is no need to use m_renderTextureNameToID in RenderPassBuilder ?
    void CreateTexture(const std::string& renderTextureName);
    void ReadTexture(const std::string& renderTextureName);

private:
    RenderPassScheduler(RenderPassID renderPassID, RenderGraphNode& node, RenderGraph& renderGraph);

    RenderTextureID GetRenderTextureID(const std::string& renderTextureName);

private:
    const RenderPassID                m_renderPassID;
    RenderTextureIDMap&               m_renderTextureNameToID;
    RenderGraphNode&                  m_renderGraphNode;
    std::vector<RenderTextureAccess>& m_renderTexturesAccess;
};


class RenderPassBuilder
{
    friend RenderGraph;

    using AttachmentUsage       = RenderGraph::AttachmentUsage;
    using RenderTextureAccess   = RenderGraph::RenderTextureAccess;
    using RenderPassID          = RenderGraph::RenderPassID;
    using RenderTextureUsageMap = RenderGraph::RenderTextureUsageMap;
    using RenderTextureIDMap    = RenderGraph::RenderTextureIDMap;

public:
    // Needed by RenderPasses
    [[nodiscard]] RenderTextureHandle GetRenderTexture(const std::string& name) const;
    [[nodiscard]] void*               GetNativeRenderTexture(RenderTextureHandle renderTextureHandle) const; // For ImGui only
    [[nodiscard]] RenderPassHandle    GetSwapchainRenderPass() const;

    [[nodiscard]] RenderTextureHandle CreateRenderTexture(
        const std::string&             name,
        ui32                           width,
        ui32                           height,
        RenderTextureFormat            format,
        const RenderTextureClearValue& clearValue
    );
    [[nodiscard]] RenderPassHandle    CreateRenderPass(
        const std::vector<RenderTextureHandle>&& colorAttachments,
        std::optional<RenderTextureHandle>       depthStencilAttachment
    );

private:
    RenderPassBuilder(RenderGraph& renderGraph);

private:
    std::unordered_map<std::string, RenderTextureHandle>& m_renderTextures;
    RenderTextureUsageMap&                                m_renderTextureUsages;
    RenderTextureIDMap&                                   m_renderTextureNameToID;
    std::vector<RenderTextureAccess>&                     m_renderTexturesAccess;
};


template<CRenderPass T>
void RenderGraph::AddRenderPass(std::string&& renderPassName)
{
    SNV_ASSERT(m_renderPassNameToID.contains(renderPassName) == false, "Trying to add RenderPass with already registered name");
    const RenderPassID renderPassID = static_cast<RenderPassID>(m_renderGraphNodes.size());
    m_renderPassNameToID[renderPassName] = renderPassID;

    auto& renderGraphNode   = m_renderGraphNodes.emplace_back(RenderGraphNode{.ReadAttachmentID = RenderTextureID::InvalidID});
    auto  renderPassBuilder = RenderPassScheduler(renderPassID, renderGraphNode, *this);
    renderGraphNode.Pass    = std::make_unique<T>(renderPassBuilder);
}

} // namespace snv
