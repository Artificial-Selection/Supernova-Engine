#include <Engine/Renderer/Renderer.hpp>
#include <Engine/Renderer/RenderContext.hpp>
#include <Engine/Renderer/RenderGraph.hpp>

#include <Engine/Renderer/RenderPasses/ResourceNames.hpp>
#include <Engine/Renderer/RenderPasses/EngineRenderPass.hpp>
#include <Engine/Renderer/RenderPasses/ImGuiRenderPass.hpp>

#include <Engine/Renderer/IRendererBackend.hpp>
#include <Engine/Renderer/OpenGL/GLBackend.hpp>
#include <Engine/Renderer/Vulkan/VulkanBackend.hpp>
#ifdef SNV_PLATFORM_WINDOWS
    #include <Engine/Renderer/Directx11/DX11Backend.hpp>
    #include <Engine/Renderer/Directx12/DX12Backend.hpp>
#endif

#include <Engine/EngineSettings.hpp>
#include <Engine/Core/Assert.hpp>

#include <Engine/Components/ComponentFactory.hpp>
#include <Engine/Components/Camera.hpp>
#include <Engine/Components/Transform.hpp>


// TODO(v.matushkin):
//  - <RenderGraphBuild>
//    There should be some way to build RenderGraph after all RenderPasses has been added, but before rendering starts


namespace snv
{

void Renderer::Init()
{
    switch (EngineSettings::GraphicsSettings.GraphicsApi)
    {
    case GraphicsApi::OpenGL:
        s_rendererBackend = new GLBackend();
        break;
    case GraphicsApi::Vulkan:
        s_rendererBackend = new VulkanBackend();
        break;
#ifdef SNV_PLATFORM_WINDOWS
    case GraphicsApi::DirectX11:
        s_rendererBackend = new DX11Backend();
        break;
    case GraphicsApi::DirectX12:
        s_rendererBackend = new DX12Backend();
        break;
#endif
    }

    // TODO(v.matushkin): <RenderGraphBuild>
    s_renderGraph = new RenderGraph();
    s_renderGraph->AddRenderPass<EngineRenderPass>("Engine");
    s_renderGraph->AddRenderPass<ImGuiRenderPass>("ImGui");
    s_renderGraph->Build(ResourceNames::EditorUI);
}

void Renderer::Shutdown()
{
    delete s_renderGraph;
    delete s_rendererBackend;
}


void Renderer::EnableBlend()
{
    s_rendererBackend->EnableBlend();
}

void Renderer::EnableDepthTest()
{
    s_rendererBackend->EnableDepthTest();
}


void* Renderer::GetNativeRenderTexture(RenderTextureHandle renderTextureHandle)
{
    return s_rendererBackend->GetNativeRenderTexture(renderTextureHandle);
}

RenderPassHandle Renderer::GetSwapchainRenderPass()
{
    return s_rendererBackend->GetSwapchainRenderPass();
}


void Renderer::SetBlendFunction(BlendMode source, BlendMode destination)
{
    s_rendererBackend->SetBlendFunction(source, destination);
}

void Renderer::SetClearColor(f32 r, f32 g, f32 b, f32 a)
{
    s_rendererBackend->SetClearColor(r, g, b, a);
}

void Renderer::SetDepthFunction(DepthCompareFunction depthCompareFunction)
{
    s_rendererBackend->SetDepthFunction(depthCompareFunction);
}

void Renderer::SetViewport(i32 x, i32 y, i32 width, i32 height)
{
    s_rendererBackend->SetViewport(x, y, width, height);
}

void Renderer::Clear(BufferBit bufferBitMask)
{
    s_rendererBackend->Clear(bufferBitMask);
}


// TODO(v.matushkin): Each mesh should use its own Transform, but since I dont have Tranform hierarchy
//   use global localToWorld
void Renderer::RenderFrame(const glm::mat4x4& localToWorld)
{
    const auto cameraView = ComponentFactory::GetView<const Camera>();
    SNV_ASSERT(cameraView.size() == 1, "The scene must have at least and only 1 camera");

    for (const auto [entity, camera] : cameraView.each())
    {
        // NOTE(v.matushkin): Can I get component through view?
        const auto& cameraTranformForReal = ComponentFactory::GetComponent<Transform>(entity);
        //const auto& cameraTransform = cameraView.get<Transform>(entity);

        // NOTE(v.matushkin): With the current architecture of render backends it is wrong to call [Begin/End]Frame per camera?
        s_rendererBackend->BeginFrame(localToWorld, cameraTranformForReal.GetMatrix(), camera.GetProjectionMatrix());
        s_renderGraph->Execute(RenderContext(s_rendererBackend));
        s_rendererBackend->EndFrame();
    }
}


IImGuiRenderContext* Renderer::CreateImGuiRenderContext()
{
    return s_rendererBackend->CreateImGuiRenderContext();
}


RenderTextureHandle Renderer::CreateRenderTexture(const RenderTextureDesc& renderTextureDesc)
{
    return s_rendererBackend->CreateRenderTexture(renderTextureDesc);
}

RenderPassHandle Renderer::CreateRenderPass(const RenderPassDesc& renderPassDesc)
{
    return s_rendererBackend->CreateRenderPass(renderPassDesc);
}


BufferHandle Renderer::CreateBuffer(
    std::span<const std::byte>              indexData,
    std::span<const std::byte>              vertexData,
    const std::vector<VertexAttributeDesc>& vertexLayout
)
{
    return s_rendererBackend->CreateBuffer(indexData, vertexData, vertexLayout);
}

TextureHandle Renderer::CreateTexture(const TextureDesc& textureDesc, const ui8* textureData)
{
    return s_rendererBackend->CreateTexture(textureDesc, textureData);
}

ShaderHandle Renderer::CreateShader(const ShaderDesc& shaderDesc)
{
    return s_rendererBackend->CreateShader(shaderDesc);
}

} // namespace snv
