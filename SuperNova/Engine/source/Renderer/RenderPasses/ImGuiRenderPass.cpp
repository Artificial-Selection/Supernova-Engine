#include <Engine/Renderer/RenderPasses/ImGuiRenderPass.hpp>
#include <Engine/Renderer/RenderPasses/ResourceNames.hpp>
#include <Engine/Renderer/RenderGraph.hpp>
#include <Engine/Renderer/RenderContext.hpp>

#include <Engine/EngineSettings.hpp>
#include <Engine/Application/Window.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>


// TODO(v.matushkin): ImGui context management


namespace snv
{

ImGuiRenderPass::ImGuiRenderPass()
{
}

ImGuiRenderPass::~ImGuiRenderPass()
{
    // NOTE(v.matushkin): ImGui context initialization is in OnCreate(), but shutdown is in destructor
    //  which works for now, but gonna break later, when I introduce RenderPass culling
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}


void ImGuiRenderPass::OnCreate(RenderGraph& renderGraph)
{
    m_engineOutput = renderGraph.GetNativeRenderTexture(ResourceNames::EngineColor);

    //- Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    //- Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    auto& style                       = ImGui::GetStyle();
    style.WindowRounding              = 0.0f;
    style.Colors[ImGuiCol_WindowBg].w = 1.0f;

    //- Setup Platform/Renderer backends
    auto glfwWindow = Window::GetGlfwWindow();
    ImGui_ImplGlfw_InitForOpenGL(glfwWindow, false);
    ImGui_ImplOpenGL3_Init("#version 460");
}

void ImGuiRenderPass::OnRender(const RenderContext& renderContext) const
{
    // TODO(v.matushkin): Hack
    renderContext.BeginRenderPass(static_cast<FramebufferHandle>(0));

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // static bool showDemoWindow = true;
    // if (showDemoWindow)
    // {
    //     ImGui::ShowDemoWindow(&showDemoWindow);
    // }

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            bool lol = false, exit = false;
    
            ImGui::MenuItem("Lol", nullptr, &lol);
            ImGui::Separator();
            ImGui::MenuItem("Exit", nullptr, &exit);

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help"))
        {
            bool noHelp = false;

            ImGui::MenuItem("No help", nullptr, &noHelp);

            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

    if (ImGui::Begin("Hierarchy"))
    {
        ImGui::End();
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    if (ImGui::Begin("Scene"))
    {
        const auto windowSize = ImGui::GetWindowSize();
        if (ImGui::BeginChild("Scene Child", windowSize))
        {
            const auto& graphicsSettings = EngineSettings::GraphicsSettings;
            ImVec2      engineOutputDimensions(graphicsSettings.RenderWidth, graphicsSettings.RenderHeight);
    
            ImGui::Image(m_engineOutput, engineOutputDimensions, ImVec2(0, 1), ImVec2(1, 0));

            ImGui::EndChild();
        }

        ImGui::End();
    }
    ImGui::PopStyleVar();

    if (ImGui::Begin("Render Stats"))
    {
        const auto framerate = ImGui::GetIO().Framerate;
        ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / framerate, framerate);

        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Update and Render additional Platform Windows
    // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere)
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
    Window::MakeContextCurrent();
}

} // namespace snv
