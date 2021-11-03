#pragma once


// TODO(v.matushkin): There should be only one ImGui context created from what I understand.
//  But right now ImGuiContext creation is not controlled in any way


namespace snv
{

struct IImGuiRenderContext;


class ImGuiContext
{
public:
    ImGuiContext();
    ~ImGuiContext();

    void BeginFrame();
    void EndFrame();

private:
    IImGuiRenderContext* m_imguiRenderContext;
};

} // namespace snv
