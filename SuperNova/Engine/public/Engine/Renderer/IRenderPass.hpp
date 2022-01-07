#pragma once

#include <concepts>


namespace snv
{

class RenderContext;
class RenderPassBuilder;


struct IRenderPass
{
    virtual ~IRenderPass() = default;

    virtual void OnCreate(RenderPassBuilder& renderPassBuilder) = 0;
    virtual void OnRender(const RenderContext& renderContext) const = 0;
};


template<class T>
concept CRenderPass = std::derived_from<T, IRenderPass>;

} // namespace snv
