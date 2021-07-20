#pragma once

#include <memory>
#include <string>
#include <unordered_map>


namespace snv
{

class Model;
class Texture;

using ModelPtr   = std::shared_ptr<Model>;
using TexturePtr = std::shared_ptr<Texture>;


// TODO(v.matushkin): For now this class is a joke
class AssetDatabase
{
    template<class T>
    using Assets   = std::unordered_map<std::string, std::shared_ptr<T>>;
    using Models   = Assets<Model>;
    using Textures = Assets<Texture>;

public:
    template<class T>
    [[nodiscard]] static std::shared_ptr<T> LoadAsset(const std::string& assetPath);

private:
    [[nodiscard]] static Model   LoadModel(const char* assetPath);
    [[nodiscard]] static Texture LoadTexture(const char* texturePath);

private:
    static Models   m_models;
    static Textures m_textures;
};

} // namespace snv
