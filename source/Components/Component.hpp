#pragma once

#include <concepts>


namespace snv
{

class BaseComponent;
class GameObject;

template<class T>
concept Component = std::derived_from<T, BaseComponent>;

// NOTE(v.matushkin): Base class or traits or something else?
class BaseComponent
{
    friend GameObject;

protected:
    /*template<Component T>
    T* GetComponent()
    {
        if (m_gameObject != nullptr)
        {
            return m_gameObject->GetComponent<T>();
        }

        return nullptr;
    }*/


    GameObject* m_gameObject;

private:
    void SetGameObject(GameObject* gameObject) { m_gameObject = gameObject; }
};



}
