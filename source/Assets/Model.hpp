#pragma once

#include <Entity/GameObject.hpp>

#include <vector>


namespace snv
{

// TODO(v.matushkin): Probably there shouldn't be a Model class,
//  it should be just some AssetDatabase.LoadAsset<GameObject>() that will load the .obj|.fbx|etc. mesh into GameObject hierarchy

class Model
{
public:
    Model(std::vector<GameObject>&& gameObjects);

    Model(Model&& other) noexcept;
    Model& operator=(Model&& other) noexcept;

    Model(const Model& other) = delete;
    Model& operator=(const Model& other) = delete;

    [[nodiscard]] const std::vector<GameObject>& GetGameObjects() const { return m_gameObjects; }

private:
    std::vector<GameObject> m_gameObjects;
};

} // namespace snv
