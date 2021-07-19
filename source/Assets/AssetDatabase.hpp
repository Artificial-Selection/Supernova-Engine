#pragma once

#include <memory>
#include <string>
#include <unordered_map>


namespace snv
{

class Model;


using ModelPtr = std::shared_ptr<Model>;

// TODO(v.matushkin): For now this class is a joke
class AssetDatabase
{
    using Models = std::unordered_map<std::string, ModelPtr>;

public:
    template<class T>
    [[nodiscard]] static std::shared_ptr<T> LoadAsset(const std::string& assetPath);

private:
    static Models m_models;
};

} // namespace snv
