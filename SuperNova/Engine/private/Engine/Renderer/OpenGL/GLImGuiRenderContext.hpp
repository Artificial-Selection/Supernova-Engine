#pragma once

#include <Engine/Core/Core.hpp>
#include <Engine/Renderer/IImGuiRenderContext.hpp>


struct ImGuiViewport;


namespace snv
{

class GLImGuiRenderContext final : public IImGuiRenderContext
{
public:
    GLImGuiRenderContext();
    ~GLImGuiRenderContext() override;

    void BeginFrame() override;
    void EndFrame(ImDrawData* imguiDrawData) override;

private:
    bool ImGui_ImplOpenGL3_Init(const char* glsl_version = nullptr);
    void ImGui_ImplOpenGL3_Shutdown();
    void ImGui_ImplOpenGL3_NewFrame();
    void ImGui_ImplOpenGL3_RenderDrawData(ImDrawData* draw_data);

    bool ImGui_ImplOpenGL3_CreateFontsTexture();
    void ImGui_ImplOpenGL3_DestroyFontsTexture();
    bool ImGui_ImplOpenGL3_CreateDeviceObjects();
    void ImGui_ImplOpenGL3_DestroyDeviceObjects();

    // For ImGui_ImplOpenGL3_CreateDeviceObjects
    bool CheckShader(ui32 handle, const char* desc);
    bool CheckProgram(ui32 handle, const char* desc);
    // For ImGui_ImplOpenGL3_RenderDrawData
    void ImGui_ImplOpenGL3_SetupRenderState(ImDrawData* draw_data, int fb_width, int fb_height, ui32 vertex_array_object);

    // MULTI-VIEWPORT / PLATFORM INTERFACE SUPPORT
    void ImGui_ImplOpenGL3_InitPlatformInterface();
    void ImGui_ImplOpenGL3_ShutdownPlatformInterface();
    static void ImGui_ImplOpenGL3_RenderWindow(ImGuiViewport* viewport, void*);

private:
    ui32 m_GlVersion;             // Extracted at runtime using GL_MAJOR_VERSION, GL_MINOR_VERSION queries (e.g. 320 for GL 3.2)
    char m_GlslVersionString[32]; // Specified by user or detected based on compile time GL settings.
    ui32 m_FontTexture;
    ui32 m_ShaderHandle;
    i32  m_AttribLocationTex;     // Uniforms location
    i32  m_AttribLocationProjMtx;
    ui32 m_AttribLocationVtxPos;  // Vertex attributes location
    ui32 m_AttribLocationVtxUV;
    ui32 m_AttribLocationVtxColor;
    ui32 m_VboHandle;
    ui32 m_ElementsHandle;
    bool m_HasClipOrigin;
};

} // namespace snv
