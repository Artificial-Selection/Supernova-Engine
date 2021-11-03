#include <Engine/Renderer/RenderContext.hpp>
#include <Engine/Renderer/IRendererBackend.hpp>

#include <Engine/Assets/Material.hpp>
#include <Engine/Assets/Mesh.hpp>
#include <Engine/Assets/Texture.hpp>

#include <Engine/Components/ComponentFactory.hpp>
#include <Engine/Components/MeshRenderer.hpp>
#include <Engine/Components/Transform.hpp>


namespace snv
{

RenderContext::RenderContext(IRendererBackend* rendererBackend)
    : m_rendererBackend(rendererBackend)
{}


void RenderContext::BeginRenderPass(FramebufferHandle framebufferHandle) const
{
    m_rendererBackend->BeginRenderPass(framebufferHandle);
}

void RenderContext::DrawRenderers() const
{
    const auto meshRendererView = ComponentFactory::GetView<const MeshRenderer>();

    for (const auto [entity, meshRenderer] : meshRendererView.each())
    {
        const auto material      = meshRenderer.GetMaterial();
        const auto textureHandle = material->GetBaseColorMap()->GetTextureHandle();

        const auto mesh        = meshRenderer.GetMesh();
        const auto meshHandle  = mesh->GetHandle();
        const auto indexCount  = mesh->GetIndexCount();
        const auto vertexCount = mesh->GetVertexCount();

        m_rendererBackend->DrawBuffer(textureHandle, meshHandle, indexCount, vertexCount);
    }
}

} // namespace snv
