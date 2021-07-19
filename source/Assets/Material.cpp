#include <Assets/Material.hpp>


namespace snv
{

Material::Material(std::shared_ptr<Texture> diffuseTexture)
    : m_diffuseTexture(diffuseTexture)
{}

} // namespace snv
