#pragma once

#include <Engine/Renderer/RenderTypes.hpp>

#include <concepts>


namespace snv
{

class RenderContext;
class RenderGraph;


struct IRenderPass
{
    virtual ~IRenderPass() = default;

    virtual void OnCreate(RenderGraph& renderGraph) = 0;
    virtual void OnRender(const RenderContext& renderContext) const = 0;
};


template<class T>
concept CRenderPass = std::derived_from<T, IRenderPass>;

} // namespace snv
