#pragma once

#include <Assets/Mesh.hpp>
#include <Assets/Material.hpp>

#include <utility>


namespace snv
{

// TODO(v.matushkin): Probably there shouldn't be a Model class,
//  it should be just some AssetDatabase.LoadAsset<GameObject>() that will load the .obj|.fbx|etc. mesh into GameObject hierarchy

class Model
{
    friend class AssetDatabase;

public:
    Model() = default;

    Model(Model&& other) noexcept;
    Model& operator=(Model&& other) noexcept;

    Model(const Model& other) = delete;
    Model& operator=(const Model& other) = delete;

    [[nodiscard]] const std::vector<std::pair<Mesh, Material>>& GetMeshes() const { return m_meshes; }

private:
    static Model LoadAsset(const char* assetPath);
    static std::pair<TextureDescriptor, ui8*> LoadTexture(const char* texturePath);

private:
    std::vector<std::pair<Mesh, Material>> m_meshes;
};

} // namespace snv
