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
    static void Init(std::string assetDirectory);

    template<class T>
    [[nodiscard]] static std::shared_ptr<T> LoadAsset(const std::string& assetPath);

private:
    [[nodiscard]] static Model   LoadModel(const std::string& modelName);
    [[nodiscard]] static Texture LoadTexture(const std::string& texturePath);
    [[nodiscard]] static Shader  LoadShader(const std::string& shaderName);

private:
    static inline std::string m_assetDir;
    static inline std::string m_modelDir;
    static inline std::string m_glShaderDir;
    static inline std::string m_vkShaderDir;
    static inline std::string m_dxShaderDir;

    static inline Models    m_models;
    static inline Textures  m_textures;
    static inline ShaderPtr m_theOneAndOnlyForNow;
};

} // namespace snv
