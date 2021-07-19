#include <Assets/AssetDatabase.hpp>
#include <Assets/Model.hpp>


namespace snv
{

std::unordered_map<std::string, std::shared_ptr<Model>> AssetDatabase::m_models;


// TODO(v.matushkin): Heterogeneous lookup for string_view assetPath?
// TODO(v.matushkin): I think assets shouldn't have *::LoadAsset() method,
//   but coding asset importers is complicated, leave it like this for now
template<>
ModelPtr AssetDatabase::LoadAsset(const std::string& assetPath)
{    
    auto modelIt = m_models.find(assetPath);
    if (modelIt == m_models.end())
    {
        modelIt = m_models.emplace(assetPath, std::make_shared<Model>(Model::LoadAsset(assetPath.c_str()))).first;
    }

    return modelIt->second;
}

} // namespace snv
