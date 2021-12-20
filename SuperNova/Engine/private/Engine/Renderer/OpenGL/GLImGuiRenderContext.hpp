#pragma once

#include <Engine/Renderer/IImGuiRenderContext.hpp>


namespace snv
{

class GLImGuiRenderContext final : public IImGuiRenderContext
{
public:
    GLImGuiRenderContext();
    ~GLImGuiRenderContext() override;

    void BeginFrame() override;
    void EndFrame(ImDrawData* imguiDrawData) override;
};

} // namespace snv
