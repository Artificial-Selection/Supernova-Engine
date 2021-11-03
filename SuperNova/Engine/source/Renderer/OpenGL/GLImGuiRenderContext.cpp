#include <Engine/Renderer/OpenGL/GLImGuiRenderContext.hpp>

#include <imgui_impl_opengl3.h>


namespace snv
{

GLImGuiRenderContext::GLImGuiRenderContext()
{
    ImGui_ImplOpenGL3_Init("#version 460");
}

GLImGuiRenderContext::~GLImGuiRenderContext()
{
    ImGui_ImplOpenGL3_Shutdown();
}


void GLImGuiRenderContext::BeginFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
}

void GLImGuiRenderContext::EndFrame(ImDrawData* imguiDrawData)
{
    ImGui_ImplOpenGL3_RenderDrawData(imguiDrawData);
}

} // namespace snv
