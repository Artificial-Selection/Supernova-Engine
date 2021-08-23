#pragma once
#include "Scene.h"
#include "GameObject.hpp"

namespace snv
{
	GameObject Scene::InstantiateGameObject()
	{
	    auto entity = GameObject(m_entityRegistry.create(), this);
		return entity;
	}

    void Scene::ReleaseGameObject(GameObject go)
	{
		m_entityRegistry.destroy(go);
	}

    void Scene::OnUpdate(float deltaTime)
    {
    }
} //namespace snv