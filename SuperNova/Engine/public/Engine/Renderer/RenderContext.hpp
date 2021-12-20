#pragma once

#include <Engine/Renderer/RenderTypes.hpp>


namespace snv
{

class IRendererBackend;


class RenderContext
{
public:
    RenderContext(IRendererBackend* rendererBackend);

    void BeginRenderPass(FramebufferHandle framebufferHandle) const;
    void BeginRenderPass(FramebufferHandle framebufferHandle, RenderTextureHandle input) const;
    void DrawRenderers() const;

private:
    IRendererBackend* const m_rendererBackend;
};

} // namespace snv
