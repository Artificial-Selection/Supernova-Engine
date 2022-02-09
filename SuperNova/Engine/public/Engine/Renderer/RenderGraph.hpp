#pragma once

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


    using RenderTextureNameToHandleMap = std::unordered_map<std::string, RenderTextureHandle>;
    using RenderTextureUsageMap        = std::unordered_map<RenderTextureID, std::stack<AttachmentUsage>>;
    using RenderPassNameIDMap          = std::unordered_map<std::string, RenderPassID>;
    using RenderTextureNameToIDMap     = std::unordered_map<std::string, RenderTextureID>;
    using RenderTextureHandleToIDMap   = std::unordered_map<RenderTextureHandle, RenderTextureID>;

public:
    RenderGraph() = default;
    ~RenderGraph() = default;

    void Build(const std::string& outputRenderTextureName);
    void Execute(const RenderContext& renderContext) const;

    void AddRenderPass(std::unique_ptr<IRenderPass> renderPass);

private:
    // Created resources
    RenderTextureNameToHandleMap     m_renderTextureNameToHandle;
    // Current render passes
    std::vector<IRenderPass*>        m_renderPasses;
    RenderTextureUsageMap            m_renderTextureUsages;
    // Render passes schedule info
    RenderPassNameIDMap              m_renderPassNameToID;
    RenderTextureNameToIDMap         m_renderTextureNameToID;
    RenderTextureHandleToIDMap       m_renderTextureHandleToID;

    std::vector<RenderGraphNode>     m_renderGraphNodes;
    std::vector<RenderTextureAccess> m_renderTexturesAccess;
};


class RenderPassScheduler
{
    friend RenderGraph;

    using RenderPassID             = RenderGraph::RenderPassID;
    using RenderTextureID          = RenderGraph::RenderTextureID;
    using RenderGraphNode          = RenderGraph::RenderGraphNode;
    using RenderTextureAccess      = RenderGraph::RenderTextureAccess;
    using RenderTextureNameToIDMap = RenderGraph::RenderTextureNameToIDMap;

public:
    // NOTE(v.matushkin): Return RenderTextureID so there is no need to use m_renderTextureNameToID in RenderPassBuilder ?
    void CreateTexture(const std::string& renderTextureName);
    void ReadTexture(const std::string& renderTextureName);

private:
    RenderPassScheduler(RenderPassID renderPassID, RenderGraphNode& node, RenderGraph& renderGraph);

    RenderTextureID GetRenderTextureID(const std::string& renderTextureName);

private:
    const RenderPassID                m_renderPassID;
    RenderTextureNameToIDMap&         m_renderTextureNameToID;
    RenderGraphNode&                  m_renderGraphNode;
    std::vector<RenderTextureAccess>& m_renderTexturesAccess;
};


class RenderPassBuilder
{
    friend RenderGraph;

    using AttachmentUsage              = RenderGraph::AttachmentUsage;
    using RenderPassID                 = RenderGraph::RenderPassID;
    using RenderTextureAccess          = RenderGraph::RenderTextureAccess;
    using RenderTextureNameToHandleMap = RenderGraph::RenderTextureNameToHandleMap;
    using RenderTextureUsageMap        = RenderGraph::RenderTextureUsageMap;
    using RenderTextureNameToIDMap     = RenderGraph::RenderTextureNameToIDMap;
    using RenderTextureHandleToIDMap   = RenderGraph::RenderTextureHandleToIDMap;

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
        const std::vector<RenderTextureHandle>& colorAttachments,
        std::optional<RenderTextureHandle>      depthStencilAttachment,
        SubpassDesc&&                           subpassDesc
    );

private:
    RenderPassBuilder(RenderGraph& renderGraph);

    [[nodiscard]] AttachmentDesc CreateAttachmentDesc(RenderTextureHandle renderTextureHandle);

private:
    RenderTextureNameToHandleMap&     m_renderTextureNameToHandle;
    RenderTextureUsageMap&            m_renderTextureUsages;
    RenderTextureNameToIDMap&         m_renderTextureNameToID;
    RenderTextureHandleToIDMap&       m_renderTextureHandleToID;
    std::vector<RenderTextureAccess>& m_renderTexturesAccess;
};

} // namespace snv
