#include <Renderer/Renderer.hpp>
#include <Renderer/IRendererBackend.hpp>
#include <Renderer/OpenGL/GLBackend.hpp>

#include <Core/Assert.hpp>

#include <Assets/Material.hpp>
#include <Assets/Mesh.hpp>
#include <Assets/Texture.hpp>

#include <Components/ComponentFactory.hpp>
#include <Components/Camera.hpp>
#include <Components/MeshRenderer.hpp>
#include <Components/Transform.hpp>


namespace snv
{

void Renderer::Init()
{
    s_RendererBackend = new GLBackend();
}

void Renderer::Shutdown()
{
    delete s_RendererBackend;
}


void Renderer::EnableBlend()
{
    s_RendererBackend->EnableBlend();
}

void Renderer::EnableDepthTest()
{
    s_RendererBackend->EnableDepthTest();
}


void Renderer::SetBlendFunction(BlendFactor source, BlendFactor destination)
{
    s_RendererBackend->SetBlendFunction(source, destination);
}

void Renderer::SetClearColor(f32 r, f32 g, f32 b, f32 a)
{
    s_RendererBackend->SetClearColor(r, g, b, a);
}

void Renderer::SetDepthFunction(DepthFunction depthFunction)
{
    s_RendererBackend->SetDepthFunction(depthFunction);
}

void Renderer::SetViewport(i32 x, i32 y, i32 width, i32 height)
{
    s_RendererBackend->SetViewport(x, y, width, height);
}

void Renderer::Clear(BufferBit bufferBitMask)
{
    s_RendererBackend->Clear(bufferBitMask);
}


// TODO(v.matushkin): Each mesh should use its own Transform, but since I dont have Tranform hierarchy
//   use global localToWorld
void Renderer::RenderFrame(const glm::mat4x4& localToWorld)
{
    const auto cameraView = ComponentFactory::GetView<const Camera>();
    SNV_ASSERT(cameraView.size() == 1, "The scene must have at least and only 1 camera");

    for (auto [entity, camera] : cameraView.each())
    {
        // NOTE(v.matushkin): Can I get component through view?
        const auto& cameraTranformForReal = ComponentFactory::GetComponent<Transform>(entity);
        //const auto& cameraTransform = cameraView.get<Transform>(entity);
        
        s_RendererBackend->StartFrame(localToWorld, cameraTranformForReal.GetMatrix(), camera.GetProjectionMatrix());
    }

    const auto meshRendererView = ComponentFactory::GetView<const MeshRenderer>();

    for (auto [entity, meshRenderer] : meshRendererView.each())
    {
        const auto material      = meshRenderer.GetMaterial();
        const auto textureHandle = material->GetBaseColorMap()->GetTextureHandle();

        const auto mesh        = meshRenderer.GetMesh();
        const auto meshHandle  = mesh->GetHandle();
        const auto indexCount  = mesh->GetIndexCount();
        const auto vertexCount = mesh->GetVertexCount();

        snv::Renderer::DrawGraphicsBuffer(textureHandle, meshHandle, indexCount, vertexCount);
    }
}


void Renderer::DrawGraphicsBuffer(
    TextureHandle textureHandle, GraphicsBufferHandle handle, i32 indexCount, i32 vertexCount
)
{
    s_RendererBackend->DrawGraphicsBuffer(textureHandle, handle, indexCount, vertexCount);
}


GraphicsBufferHandle Renderer::CreateGraphicsBuffer(
    std::span<const std::byte> indexData,
    std::span<const std::byte> vertexData,
    const std::vector<VertexAttributeDescriptor>& vertexLayout
)
{
    return s_RendererBackend->CreateGraphicsBuffer(indexData, vertexData, vertexLayout);
}

TextureHandle Renderer::CreateTexture(const TextureDescriptor& textureDescriptor, const ui8* data)
{
    return s_RendererBackend->CreateTexture(textureDescriptor, data);
}

ShaderHandle Renderer::CreateShader(const char* vertexSource, const char* fragmentSource)
{
    return s_RendererBackend->CreateShader(vertexSource, fragmentSource);
}

} // namespace snv
