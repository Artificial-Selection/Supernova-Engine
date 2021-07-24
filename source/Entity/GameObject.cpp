#include <Entity/GameObject.hpp>
#include <Components/Transform.hpp>


namespace snv
{

GameObject::GameObject()
    : m_entity(ComponentFactory::CreateEntity())
{
    ComponentFactory::AddComponent<Transform>(m_entity, this);
}

} // namespace snv
