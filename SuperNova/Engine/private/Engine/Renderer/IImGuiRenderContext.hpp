#pragma once


// TODO(v.matushkin): Right now this *ImGuiRenderContext classes are simple wrappers,
//  but one day I hope I'm gonna replace this <imgui_impl_*.h> headers with my own implementations


struct ImDrawData;


namespace snv
{

struct IImGuiRenderContext
{
    virtual ~IImGuiRenderContext() = default;

    virtual void BeginFrame() = 0;
    virtual void EndFrame(ImDrawData* imguiDrawData) = 0;
};

} // namespace snv
