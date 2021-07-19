#include <Assets/AssetDatabase.hpp>
#include <Assets/Model.hpp>
#include <Assets/Texture.hpp>


namespace snv
{

AssetDatabase::Models   AssetDatabase::m_models;
AssetDatabase::Textures AssetDatabase::m_textures;


// TODO(v.matushkin): Heterogeneous lookup for string_view assetPath?
// TODO(v.matushkin): I think assets shouldn't have *::LoadAsset() method,
//   but coding asset importers is complicated, leave it like this for now
template<>
ModelPtr AssetDatabase::LoadAsset(const std::string& assetPath)
{
    return LoadAssetInternal(assetPath, m_models);
}

template<>
TexturePtr AssetDatabase::LoadAsset(const std::string& assetPath)
{
    return LoadAssetInternal(assetPath, m_textures);
}

} // namespace snv
