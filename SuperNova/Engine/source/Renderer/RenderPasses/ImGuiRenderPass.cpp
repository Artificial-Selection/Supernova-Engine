#include <Engine/Renderer/RenderPasses/ImGuiRenderPass.hpp>
#include <Engine/Renderer/RenderPasses/ResourceNames.hpp>
#include <Engine/Renderer/RenderContext.hpp>
#include <Engine/Renderer/RenderGraph.hpp>

#include <Engine/EngineSettings.hpp>
#include <Engine/Application/Window.hpp>
#include <Engine/UI/ImGuiContext.hpp>


#include <Engine/Core/Log.hpp> // TODO(v.matushkin): Remove


#include <imgui.h>


// TODO(v.matushkin):
//  - <ImageUV>
//    I need to set UV for ImGui::Image() when using OpenGL, can I get rid of this?
//    May be change the shader in GLImGuiRenderContext to swap UV? Don't know if there is any other option.


namespace snv
{

ImGuiRenderPass::ImGuiRenderPass(RenderPassScheduler& renderPassScheduler)
    : m_imguiContext(nullptr)
    , m_swapchainRenderPassHandle(RenderPassHandle::InvalidHandle)
    , m_engineOutputRenderTexture(RenderTextureHandle::InvalidHandle)
    , m_engineOutputNativeRenderTexture(nullptr)
{
    LOG_INFO("ImGuiRenderPass::ImGuiRenderPass");
    // TODO(v.matushkin): This is a hack until I can teach RenderGraph how to connect its passes to swapchain
    renderPassScheduler.CreateTexture(ResourceNames::EditorUI);
    renderPassScheduler.ReadTexture(ResourceNames::EngineColor);
}

ImGuiRenderPass::~ImGuiRenderPass()
{
    delete m_imguiContext; // NOTE(v.matushkin): May be put this in OnDestroy() (if I make this method)
}


void ImGuiRenderPass::OnCreate(RenderPassBuilder& renderPassBuilder)
{
    LOG_INFO("ImGuiRenderPass::OnCreate");
    m_swapchainRenderPassHandle       = renderPassBuilder.GetSwapchainRenderPass();
    m_engineOutputRenderTexture       = renderPassBuilder.GetRenderTexture(ResourceNames::EngineColor);
    m_engineOutputNativeRenderTexture = renderPassBuilder.GetNativeRenderTexture(m_engineOutputRenderTexture);

    m_imguiContext = new ImGuiContext();
}

void ImGuiRenderPass::OnRender(const RenderContext& renderContext) const
{
    renderContext.BeginRenderPass(m_swapchainRenderPassHandle, m_engineOutputRenderTexture);
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
            // NOTE(v.matushkin): Do I need to get this every frame?
            const auto& graphicsSettings = EngineSettings::GraphicsSettings;
            ImVec2      engineOutputDimensions(graphicsSettings.RenderWidth, graphicsSettings.RenderHeight);

            // TODO(v.matushkin): <ImageUV>
            if (graphicsSettings.GraphicsApi == GraphicsApi::OpenGL)
            {
                ImGui::Image(m_engineOutputNativeRenderTexture, engineOutputDimensions, ImVec2(0, 1), ImVec2(1, 0));
            }
            else
            {
                ImGui::Image(m_engineOutputNativeRenderTexture, engineOutputDimensions);
            }

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
    renderContext.EndRenderPass();
}

} // namespace snv
