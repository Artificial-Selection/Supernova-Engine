#pragma once
#include "entt/entt.hpp"
#include <Components/Component.hpp>
#include <utility>

namespace snv
{
//NOTE(i.nazarov): should it be divided into physic scene/graphic scene/etc?
class Scene final
{
public:
    Scene() = default;
    ~Scene() = default;

#pragma region TemplateMethods
        template<Component T, typename... Args>
    T& AddComponent(const entt::entity entity, GameObject* gameObject, Args&&... args)
    {
        return m_entityRegistry.emplace<T>(entity, gameObject, std::forward<Args>(args)...);
    }

    template<Component T>
     T& GetComponent(const entt::entity entity)
    {
        return m_entityRegistry.get<T>(entity);
    }

    // NOTE(v.matushkin): I'm pretty sure Exclude will not work, since yuo need to pass it to m_registry.view() ?
    template<Component... T, class... Exclude>
     entt::basic_view<entt::entity, entt::exclude_t<Exclude...>, T...> GetView(entt::exclude_t<Exclude...> = {})
    {
        return m_entityRegistry.view<T..., Exclude...>();
    }
#pragma endregion TemplateMethods

    GameObject InstantiateGameObject();

    void ReleaseGameObject(GameObject go);

    void OnUpdate(float deltaTime);

private:
    entt::registry m_entityRegistry;

private:
    friend class GameObject;
};
}
