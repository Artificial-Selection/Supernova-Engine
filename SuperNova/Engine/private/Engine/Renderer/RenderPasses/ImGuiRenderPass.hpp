#pragma once

#include <Engine/Renderer/IRenderPass.hpp>


namespace snv
{

class ImGuiContext;
class RenderGraph;
class RenderContext;


// TODO(v.matushkin): Move this pass to the Editor code. Rename to something like EditorUIRenderPass ?
class ImGuiRenderPass : public IRenderPass
{
public:
    ImGuiRenderPass();
    ~ImGuiRenderPass();

    void OnCreate(RenderGraph& renderGraph) override;
    void OnRender(const RenderContext& renderContext) const override;

private:
    ImGuiContext* m_imguiContext;

    void* m_engineOutputRenderTexture;
};

} // namespace snv
