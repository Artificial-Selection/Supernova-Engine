#pragma once

#include <Engine/Renderer/IRenderPass.hpp>
#include <Engine/Renderer/RenderTypes.hpp>


namespace snv
{

class ImGuiContext;
class RenderContext;
class RenderPassBuilder;
class RenderPassScheduler;


// TODO(v.matushkin): Move this pass to the Editor code. Rename to something like EditorUIRenderPass ?
class ImGuiRenderPass final : public IRenderPass
{
public:
    ImGuiRenderPass();
    ~ImGuiRenderPass();

    const std::string& GetName() const override { return m_name; }

    void OnSchedule(RenderPassScheduler& renderPassScheduler) const override;
    void OnCreate(RenderPassBuilder& renderPassBuilder) override;
    void OnRender(const RenderContext& renderContext) const override;

private:
    const std::string   m_name;

    ImGuiContext*       m_imguiContext;

    RenderPassHandle    m_swapchainRenderPassHandle;
    RenderTextureHandle m_engineOutputRenderTexture;
    void*               m_engineOutputNativeRenderTexture;
};

} // namespace snv
