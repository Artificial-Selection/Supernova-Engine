//
// Created by Devilast on 6/27/2021.
//

#pragma once

#include <Engine/Components/Component.hpp>

#include <entt/entity/registry.hpp>
#include <entt/entity/view.hpp>

#include <utility>


namespace snv
{

class GameObject;


class ComponentFactory
{
public:
    static entt::entity CreateEntity();

    template<Component T, typename... Args>
    static T& AddComponent(const entt::entity entity, GameObject* gameObject, Args&&... args)
    {
        return m_registry.emplace<T>(entity, gameObject, std::forward<Args>(args)...);
    }

    template<Component T>
    static T& GetComponent(const entt::entity entity)
    {
        return m_registry.get<T>(entity);
    }

    // NOTE(v.matushkin): I'm pretty sure Exclude will not work, since yuo need to pass it to m_registry.view() ?
    template<Component... T, class... Exclude>
    static entt::basic_view<entt::entity, entt::exclude_t<Exclude...>, T...> GetView(entt::exclude_t<Exclude...> = {})
    {
        return m_registry.view<T..., Exclude...>();
    }

private:
    static inline entt::registry m_registry;
};

} // namespace snv
