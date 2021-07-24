#include <Components/MeshRenderer.hpp>


namespace snv
{

MeshRenderer::MeshRenderer(GameObject* gameObject, std::shared_ptr<Material> material, std::shared_ptr<Mesh> mesh)
    : BaseComponent(gameObject)
    , m_material(material)
    , m_mesh(mesh)
{}

} // namespace snv
