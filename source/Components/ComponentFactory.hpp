//
// Created by Devilast on 6/27/2021.
//

#pragma once
#include <Core/Utils/Singleton.hpp>
#include <entt/entt.hpp>

#include <utility>


namespace snv
{

class ComponentFactory : public Singleton<ComponentFactory>
{
public:
    entt::entity CreateEntity();

    template<typename Component, typename... Args>
    Component& AddComponent(const entt::entity entity, Args&&... args)
    {
        return m_registry.emplace<Component>(entity, std::forward<Args>(args)...);
    }

    template<class Component>
    Component& GetComponent(const entt::entity entity)
    {
        return m_registry.get<Component>(entity);
    }

    template<typename... Component, typename... Exclude>
    entt::basic_view<entt::exclude_t<Exclude...>, Component...> GetViewByComponents(entt::exclude_t<Exclude...> = {})
    {
        return m_registry.view<Component..., Exclude...>();
    }

private:
    entt::registry m_registry;
};

} // namespace snv
