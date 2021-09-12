#include <Engine/Renderer/Renderer.hpp>

#include <Engine/Renderer/IRendererBackend.hpp>
#include <Engine/Renderer/OpenGL/GLBackend.hpp>
#include <Engine/Renderer/Vulkan/VKBackend.hpp>
#ifdef SNV_PLATFORM_WINDOWS
    #include <Engine/Renderer/Directx11/DX11Backend.hpp>
    #include <Engine/Renderer/Directx12/DX12Backend.hpp>
#endif

#include <Engine/Core/Assert.hpp>

#include <Engine/Assets/Material.hpp>
#include <Engine/Assets/Mesh.hpp>
#include <Engine/Assets/Texture.hpp>

#include <Engine/Components/ComponentFactory.hpp>
#include <Engine/Components/Camera.hpp>
#include <Engine/Components/MeshRenderer.hpp>
#include <Engine/Components/Transform.hpp>


namespace snv
{

void Renderer::Init(GraphicsApi graphicsApi)
{
    switch (graphicsApi)
    {
    case GraphicsApi::OpenGL:
        s_rendererBackend = new GLBackend();
        break;
    case GraphicsApi::Vulkan:
        s_rendererBackend = new VKBackend();
        break;
#ifdef SNV_PLATFORM_WINDOWS
    case GraphicsApi::DirectX11:
        s_rendererBackend = new DX11Backend();
        break;
    case GraphicsApi::DirectX12:
        s_rendererBackend = new DX12Backend();
        break;
#endif
    default:
        SNV_ASSERT(false, "Will this ever happen?");
        break;
    }

    s_graphicsApi = graphicsApi;
}

void Renderer::Shutdown()
{
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


void Renderer::SetBlendFunction(BlendFactor source, BlendFactor destination)
{
    s_rendererBackend->SetBlendFunction(source, destination);
}

void Renderer::SetClearColor(f32 r, f32 g, f32 b, f32 a)
{
    s_rendererBackend->SetClearColor(r, g, b, a);
}

void Renderer::SetDepthFunction(DepthFunction depthFunction)
{
    s_rendererBackend->SetDepthFunction(depthFunction);
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
    const auto meshRendererView = ComponentFactory::GetView<const MeshRenderer>();

    for (const auto [entity, camera] : cameraView.each())
    {
        // NOTE(v.matushkin): Can I get component through view?
        const auto& cameraTranformForReal = ComponentFactory::GetComponent<Transform>(entity);
        //const auto& cameraTransform = cameraView.get<Transform>(entity);

        s_rendererBackend->BeginFrame(localToWorld, cameraTranformForReal.GetMatrix(), camera.GetProjectionMatrix());

        for (const auto [entity, meshRenderer] : meshRendererView.each())
        {
            const auto material      = meshRenderer.GetMaterial();
            const auto textureHandle = material->GetBaseColorMap()->GetTextureHandle();

            const auto mesh        = meshRenderer.GetMesh();
            const auto meshHandle  = mesh->GetHandle();
            const auto indexCount  = mesh->GetIndexCount();
            const auto vertexCount = mesh->GetVertexCount();

            s_rendererBackend->DrawBuffer(textureHandle, meshHandle, indexCount, vertexCount);
        }

        s_rendererBackend->EndFrame();
    }
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

ShaderHandle Renderer::CreateShader(std::span<const char> vertexSource, std::span<const char> fragmentSource)
{
    return s_rendererBackend->CreateShader(vertexSource, fragmentSource);
}

} // namespace snv
