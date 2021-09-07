#include <Editor/Editor.hpp>

// #include <Engine/Application/Window.hpp>

// #include <imgui.h>
// #include <imgui_impl_glfw.h>
// #include <imgui_impl_opengl3.h>


namespace snv
{

void Editor::OnCreate()
{
    // IMGUI_CHECKVERSION();
    // ImGui::CreateContext();
    // ImGui::StyleColorsDark();

    // NOTE(v.matushkin): What should I set for install_callbacks?
    // ImGui_ImplGlfw_InitForOpenGL(Window::GetGlfwWindow(), false);
    // ImGui_ImplOpenGL3_Init("#version 460");
}

void Editor::OnDestroy()
{
    // ImGui_ImplOpenGL3_Shutdown();
    // ImGui_ImplGlfw_Shutdown();
    // ImGui::DestroyContext();
}

void Editor::OnUpdate()
{
}

} // namespace snv
