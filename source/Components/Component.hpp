#pragma once

#include <concepts>


namespace snv
{

class BaseComponent;
class GameObject;

template<class T>
concept Component = std::derived_from<T, BaseComponent>;

// NOTE(v.matushkin): May be components shouldn't have acces to GameObject
//  And stuff like CameraController should be a system not a component

// TODO(v.matushkin): This shit with GameObject pointer is so retarded, but fuck it :)
//   No smart pointers, no checks if GameObject still alive(although I guess entt will delete component so there is no need to check?)

class BaseComponent
{
protected:
    BaseComponent(GameObject* gameObject) : m_gameObject(gameObject) {}

protected:
    GameObject* m_gameObject;
};

} // namespace snv
