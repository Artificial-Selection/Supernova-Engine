#pragma once

#include "Scene.h"

#include <Components/ComponentFactory.hpp>
#include <Components/Component.hpp>

class Scene;

namespace snv
{

class GameObject
{
public:
    GameObject() = default;

    GameObject(entt::entity entityId, Scene* attachedScene);

    GameObject(const GameObject& otherGo) = default;

    template<Component T, typename... Args>
    T& AddComponent(Args&&... args)
    {
        return ComponentFactory::AddComponent<T>(m_entity, this, std::forward<Args>(args)...);
    }

    template<Component T>
    T& GetComponent() const
    {
        return ComponentFactory::GetComponent<T>(m_entity);
    }

    std::string GetName() const
    {
        return m_name;
    }

    void SetName(std::string name)
    {
        m_name = name;
    }

    entt::entity& operator() ()
    {
        return m_entity;
    }

private:

    entt::entity m_entity;
    std::string m_name = std::string();
    Scene* m_scene = nullptr;
};

}
