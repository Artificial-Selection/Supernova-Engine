#pragma once

#include <Engine/Renderer/IRenderPass.hpp>
#include <Engine/Renderer/RenderTypes.hpp>


namespace snv
{

class RenderContext;
class RenderPassBuilder;
class RenderPassScheduler;


class EngineRenderPass final : public IRenderPass
{
public:
    EngineRenderPass(RenderPassScheduler& renderPassScheduler);

    void OnCreate(RenderPassBuilder& renderPassBuilder) override;
    void OnRender(const RenderContext& renderContext) const override;

private:
    RenderPassHandle m_renderPassHandle;
};

} // namespace snv
