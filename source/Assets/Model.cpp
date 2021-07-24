#include <Assets/Model.hpp>


namespace snv
{

Model::Model(std::vector<GameObject>&& gameObjects)
    : m_gameObjects(std::move(gameObjects))
{}

Model::Model(Model&& other) noexcept
    : m_gameObjects(std::move(other.m_gameObjects))
{}

Model& Model::operator=(Model&& other) noexcept
{
    m_gameObjects = std::move(other.m_gameObjects);

    return *this;
}

} // namespace snv
