#include <Entity/GameObject.hpp>
#include <Components/Transform.hpp>
#include "Scene.h"

namespace snv
{

GameObject::GameObject(entt::entity entityId, Scene* attachedScene) : m_entity(entityId), m_scene(attachedScene)
{
}

} // namespace snv
