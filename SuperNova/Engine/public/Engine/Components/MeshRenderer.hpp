#pragma once

#include <Engine/Components/Component.hpp>

#include <memory>


namespace snv
{

class Material;
class Mesh;


class MeshRenderer final : public BaseComponent
{
public:
    MeshRenderer(GameObject* gameObject, std::shared_ptr<Material> material, std::shared_ptr<Mesh> mesh);

    [[nodiscard]] std::shared_ptr<Material> GetMaterial() const { return m_material; }
    [[nodiscard]] std::shared_ptr<Mesh>     GetMesh()     const { return m_mesh; }

private:
    std::shared_ptr<Material> m_material;
    std::shared_ptr<Mesh>     m_mesh;
};

} // namespace snv
