#include <Engine/Renderer/RenderPasses/ImGuiRenderPass.hpp>
#include <Engine/Renderer/RenderPasses/ResourceNames.hpp>
#include <Engine/Renderer/RenderGraph.hpp>
#include <Engine/Renderer/RenderContext.hpp>

#include <Engine/EngineSettings.hpp>
#include <Engine/Application/Window.hpp>
#include <Engine/UI/ImGuiContext.hpp>

#include <imgui.h>


namespace snv
{

ImGuiRenderPass::ImGuiRenderPass()
    : m_imguiContext(nullptr)
    , m_engineOutputRenderTexture(nullptr)
{}

ImGuiRenderPass::~ImGuiRenderPass()
{
    delete m_imguiContext; // NOTE(v.matushkin): May be put this in OnDestroy() (if I make this method)
}


void ImGuiRenderPass::OnCreate(RenderGraph& renderGraph)
{
    m_engineOutputRenderTexture = renderGraph.GetNativeRenderTexture(ResourceNames::EngineColor);

    m_imguiContext = new ImGuiContext();
}

void ImGuiRenderPass::OnRender(const RenderContext& renderContext) const
{
    // TODO(v.matushkin): Hack
    renderContext.BeginRenderPass(static_cast<FramebufferHandle>(0));

    m_imguiContext->BeginFrame();

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
    
            ImGui::Image(m_engineOutputRenderTexture, engineOutputDimensions, ImVec2(0, 1), ImVec2(1, 0));

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

    m_imguiContext->EndFrame();
}

} // namespace snv
