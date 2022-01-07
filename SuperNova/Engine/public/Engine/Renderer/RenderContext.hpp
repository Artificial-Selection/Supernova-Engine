#pragma once

#include <Engine/Renderer/RenderTypes.hpp> // NOTE(v.matuhskin): Forward?


namespace snv
{

class IRendererBackend;


class RenderContext
{
public:
    RenderContext(IRendererBackend* rendererBackend);

    void BeginRenderPass(RenderPassHandle renderPassHandle) const;
    void BeginRenderPass(RenderPassHandle renderPassHandle, RenderTextureHandle input) const;
    void EndRenderPass() const;

    void DrawRenderers() const;

private:
    IRendererBackend* const m_rendererBackend;
};

} // namespace snv
