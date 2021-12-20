#pragma once

#include <Engine/Renderer/IRenderPass.hpp>


namespace snv
{

class RenderGraph;
class RenderContext;


class EngineRenderPass final : public IRenderPass
{
public:
    EngineRenderPass();

    void OnCreate(RenderGraph& renderGraph) override;
    void OnRender(const RenderContext& renderContext) const override;

private:
    FramebufferHandle m_framebufferHandle;
};

} // namespace snv
