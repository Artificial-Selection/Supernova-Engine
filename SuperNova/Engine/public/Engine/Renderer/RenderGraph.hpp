#pragma once

#include <Engine/Renderer/IRenderPass.hpp>

#include <string>
#include <unordered_map>
#include <vector>


namespace snv
{

class RenderContext;


class RenderGraph
{
public:
    RenderGraph(std::vector<IRenderPass*>&& renderPasses);

    void Execute(const RenderContext& renderContext) const;

    // Needed by RenderPasses
    [[nodiscard]] RenderTextureHandle GetRenderTexture(const std::string& name) const { return m_renderTextures.at(name); }
    [[nodiscard]] void*               GetNativeRenderTexture(const std::string& name) const;
    [[nodiscard]] FramebufferHandle   GetSwapchainFramebuffer() const;

    [[nodiscard]] GraphicsState CreateGraphicsState(
        const GraphicsStateDesc& graphicsStateDesc,
        const std::vector<std::string>& attachmentNames
    );

private:
    std::vector<IRenderPass*> m_renderPasses;

    // std::unordered_map<std::string, FramebufferHandle>   m_framebuffers;
    std::unordered_map<std::string, RenderTextureHandle> m_renderTextures;
};

} // namespace snv
