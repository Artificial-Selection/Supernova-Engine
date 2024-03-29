#pragma once

#include <Engine/Components/ComponentFactory.hpp>
#include <Engine/Components/Component.hpp>


namespace snv
{

class GameObject
{
public:
    GameObject();

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

private:
    entt::entity m_entity;
};

}
