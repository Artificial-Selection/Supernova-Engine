#include <Engine/Renderer/RenderContext.hpp>
#include <Engine/Renderer/IRendererBackend.hpp>

#include <Engine/Assets/Material.hpp>
#include <Engine/Assets/Mesh.hpp>
#include <Engine/Assets/Texture.hpp>
#include <Engine/Assets/Shader.hpp>

#include <Engine/Components/ComponentFactory.hpp>
#include <Engine/Components/MeshRenderer.hpp>
#include <Engine/Components/Transform.hpp>


namespace snv
{

RenderContext::RenderContext(IRendererBackend* rendererBackend)
    : m_rendererBackend(rendererBackend)
{}


void RenderContext::BeginRenderPass(RenderPassHandle renderPassHandle) const
{
    m_rendererBackend->BeginRenderPass(renderPassHandle);
}

void RenderContext::BeginRenderPass(RenderPassHandle renderPassHandle, RenderTextureHandle input) const
{
    m_rendererBackend->BeginRenderPass(renderPassHandle, input);
}

void RenderContext::EndRenderPass() const
{
    m_rendererBackend->EndRenderPass();
}


void RenderContext::DrawRenderers() const
{
    const auto meshRendererView = ComponentFactory::GetView<const MeshRenderer>();

    // TODO(v.matushkin): Remove this. For now it works because there is only one shader used for GameObjects
    const auto& firstMeshRenderer = meshRendererView.get<const MeshRenderer>(meshRendererView.front());
    const auto shaderHandle = firstMeshRenderer.GetMaterial()->GetShader()->GetHandle();
    m_rendererBackend->BindShader(shaderHandle);

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
