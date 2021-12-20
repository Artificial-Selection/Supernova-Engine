#pragma once

#include <Engine/Renderer/IRenderPass.hpp>


namespace snv
{

class ImGuiContext;
class RenderGraph;
class RenderContext;


// TODO(v.matushkin): Move this pass to the Editor code. Rename to something like EditorUIRenderPass ?
class ImGuiRenderPass final : public IRenderPass
{
public:
    ImGuiRenderPass();
    ~ImGuiRenderPass();

    void OnCreate(RenderGraph& renderGraph) override;
    void OnRender(const RenderContext& renderContext) const override;

private:
    ImGuiContext*       m_imguiContext;

    FramebufferHandle   m_swapchainFramebuffer;
    RenderTextureHandle m_engineOutputRenderTexture;
    void*               m_engineOutputNativeRenderTexture;
};

} // namespace snv
