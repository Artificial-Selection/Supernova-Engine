#pragma once

#include <Components/ComponentFactory.hpp>
#include <Components/Component.hpp>


namespace snv
{

class GameObject
{
public:
    GameObject();

    template<Component T, typename... Args>
    T& AddComponent(Args&&... args)
    {
        T& component = ComponentFactory::Instance().AddComponent<T>(m_entity, std::forward<Args>(args)...);
        component.SetGameObject(this);
        return component;
    }

    template<Component T>
    T& GetComponent()
    {
        
        return ComponentFactory::Instance().GetComponent<T>(m_entity);
    }

private:
    entt::entity m_entity;
};

}
