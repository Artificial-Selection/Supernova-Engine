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
    EngineRenderPass();

    const std::string& GetName() const override { return m_name; }

    void OnSchedule(RenderPassScheduler& renderPassScheduler) const override;
    void OnCreate(RenderPassBuilder& renderPassBuilder) override;
    void OnRender(const RenderContext& renderContext) const override;

private:
    const std::string m_name;

    RenderPassHandle  m_renderPassHandle;
};

} // namespace snv
