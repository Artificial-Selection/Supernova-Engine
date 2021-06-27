//
// Created by Devilast on 6/27/2021.
//

#pragma once
#include <Core/Utils/Singleton.hpp>
#include <entt/entt.hpp>

//class Singleton;

template<typename Entity>
class ComponentFactory : public Singleton<ComponentFactory<Entity>>
{
public:
    template<typename... Component, typename... Exclude>
    entt::basic_view<Entity, entt::exclude_t<Exclude...>, Component...> GetViewByComponents(entt::exclude_t<Exclude...> = {})
    {
        return _registry.view<Component... , Exclude...>();
    }

    template<typename Component, typename... Args>
    decltype(auto) Emplace(Entity entity, Args&&... args)
    {
        return _registry.emplace<Component>(entity, args...);
    }

    Entity Create()
    {
        return _registry.create();
    }

private:
    entt::registry _registry;
};
