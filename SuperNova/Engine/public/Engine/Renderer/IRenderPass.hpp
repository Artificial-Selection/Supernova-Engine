#pragma once

#include <string>


namespace snv
{

class RenderContext;
class RenderPassBuilder;
class RenderPassScheduler;


struct IRenderPass
{
    virtual ~IRenderPass() = default;

    // TODO(v.matushkin): Remove names from RenderPasses in final/production build? They're mostly needed for debugging
    virtual const std::string& GetName() const = 0;

    virtual void OnSchedule(RenderPassScheduler& renderPassScheduler) const = 0;
    virtual void OnCreate(RenderPassBuilder& renderPassBuilder) = 0;
    virtual void OnRender(const RenderContext& renderContext) const = 0;
};

} // namespace snv
