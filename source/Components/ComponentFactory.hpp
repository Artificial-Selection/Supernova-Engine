//
// Created by Devilast on 6/27/2021.
//

#pragma once
#include <Core/Utils/Singleton.hpp>
#include <entt/entt.hpp>

#include <utility>


namespace snv
{

class GameObject;


class ComponentFactory : public Singleton<ComponentFactory>
{
public:
    entt::entity CreateEntity();

    template<typename T, typename... Args>
    T& AddComponent(const entt::entity entity, GameObject* gameObject, Args&&... args)
    {
        return m_registry.emplace<T>(entity, gameObject, std::forward<Args>(args)...);
    }

    template<class T>
    T& GetComponent(const entt::entity entity)
    {
        return m_registry.get<T>(entity);
    }

    template<typename... T, typename... Exclude>
    entt::basic_view<entt::exclude_t<Exclude...>, T...> GetViewByComponents(entt::exclude_t<Exclude...> = {})
    {
        return m_registry.view<T..., Exclude...>();
    }

private:
    entt::registry m_registry;
};

} // namespace snv
