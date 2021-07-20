#include <Assets/Model.hpp>


namespace snv
{

Model::Model(std::vector<std::pair<Mesh, Material>>&& meshes)
    : m_meshes(std::move(meshes))
{}

Model::Model(Model&& other) noexcept
    : m_meshes(std::move(other.m_meshes))
{}

Model& Model::operator=(Model&& other) noexcept
{
    m_meshes = std::move(other.m_meshes);

    return *this;
}

} // namespace snv
