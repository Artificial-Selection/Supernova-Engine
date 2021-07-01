#pragma once

#include <Components/ComponentFactory.hpp>


namespace snv
{

class GameObject
{
public:
    GameObject();

    template<class Component>
    Component& AddComponent()
    {
        return ComponentFactory::Instance().AddComponent<Component>(m_entity);
    }

    template<class Component>
    Component& GetComponent()
    {
        return ComponentFactory::Instance().GetComponent<Component>(m_entity);
    }

private:
    entt::entity m_entity;
};

}
