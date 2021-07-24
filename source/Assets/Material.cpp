#include <Assets/Material.hpp>

#include <utility>


namespace snv
{

Material::Material(std::shared_ptr<Shader> shader)
    : m_shader(shader)
{}


void Material::SetName(std::string name)
{
    m_materialName = std::move(name);
}

} // namespace snv
