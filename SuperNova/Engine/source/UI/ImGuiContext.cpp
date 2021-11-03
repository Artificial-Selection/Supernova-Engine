#include <Engine/UI/ImGuiContext.hpp>

#include <Engine/EngineSettings.hpp>
#include <Engine/Application/Window.hpp>
#include <Engine/Renderer/Renderer.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>


namespace snv
{

ImGuiContext::ImGuiContext()
{
    //- Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable;

    //- Setup Dear ImGui style
    ImGui::StyleColorsDark(); // NOTE(v.matushkin): Probably not a good idea to set style here, but rn it doesn't matter

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    auto& style = ImGui::GetStyle();
    style.WindowRounding              = 0.0f;
    style.Colors[ImGuiCol_WindowBg].w = 1.0f;

    //- Setup Platform/Renderer backends
    // TODO(v.matushkin): ImGui_ImplGlfw_InitForOther probably not gonna workd for DX11/DX12
    //  Need to look at ImGui_ImplSDL2_InitForD3D/ImGui_ImplWin32_Init if they are doing something special
    //  It's interesting that example_glfw_metal using ImGui_ImplGlfw_InitForOpenGL()
    auto* glfwWindow = Window::GetGlfwWindow();
    switch (EngineSettings::GraphicsSettings.GraphicsApi)
    {
    case GraphicsApi::OpenGL:
        ImGui_ImplGlfw_InitForOpenGL(glfwWindow, false);
        break;
    case GraphicsApi::Vulkan:
        ImGui_ImplGlfw_InitForVulkan(glfwWindow, false);
        break;
#ifdef SNV_PLATFORM_WINDOWS
    case GraphicsApi::DirectX11:
    case GraphicsApi::DirectX12:
        ImGui_ImplGlfw_InitForOther(glfwWindow, false);
        break;
#endif
    }

    m_imguiRenderContext = Renderer::CreateImGuiRenderContext();
}

ImGuiContext::~ImGuiContext()
{
    delete m_imguiRenderContext;
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}


void ImGuiContext::BeginFrame()
{
    // NOTE(v.matushkin): Isn't it a little bit weird that I need to call GAPI_NewFrame() before Glfw_NeFrame() ?
    m_imguiRenderContext->BeginFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiContext::EndFrame()
{
    ImGui::Render();
    m_imguiRenderContext->EndFrame(ImGui::GetDrawData());

    // Update and Render additional Platform Windows
    // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code
    // elsewhere)
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
    // NOTE(v.matushkin): Can I get rid of this shit one day?
    if (EngineSettings::GraphicsSettings.GraphicsApi == GraphicsApi::OpenGL)
    {
        Window::MakeContextCurrent();
    }
}

} // namespace snv
