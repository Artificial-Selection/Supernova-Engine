#pragma once

#include <memory>
#include <string>
#include <unordered_map>


namespace snv
{

class Model;
class Texture;
class Shader;

using ModelPtr   = std::shared_ptr<Model>;
using TexturePtr = std::shared_ptr<Texture>;
using ShaderPtr  = std::shared_ptr<Shader>;


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
    [[nodiscard]] static Model   LoadModel(const char* modelPath);
    [[nodiscard]] static Texture LoadTexture(const std::string& texturePath);
    [[nodiscard]] static Shader  LoadShader(const std::string& shaderPath);

private:
    static inline Models    m_models;
    static inline Textures  m_textures;
    static inline ShaderPtr m_theOneAndOnlyForNow;
};

} // namespace snv
