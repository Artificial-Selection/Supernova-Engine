#include <Entity/GameObject.hpp>
#include <Components/Transform.hpp>


namespace snv
{

GameObject::GameObject()
    : m_entity(ComponentFactory::Instance().CreateEntity())
{
    ComponentFactory::Instance().AddComponent<Transform>(m_entity);
}

} // namespace snv
