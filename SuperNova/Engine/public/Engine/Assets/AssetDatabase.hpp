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


// TODO(v.matushkin): For now this class is a joke. It should be moved to Editor code and AssetManager class added to the Engine
class AssetDatabase
{
    template<class T>
    using Assets   = std::unordered_map<std::string, std::shared_ptr<T>>;
    using Models   = Assets<Model>;
    using Textures = Assets<Texture>;
    using Shaders  = Assets<Shader>;

public:
    static void Init(std::string&& assetDirectory);

    // TODO(v.matushkin): It's bad to not define template method in the header, at least do something like in the EnumUtils.hpp
    //  And probably I should add base class for assets.
    template<class T>
    [[nodiscard]] static std::shared_ptr<T> LoadAsset(const std::string& assetPath);

private:
    [[nodiscard]] static Model   LoadModel(const std::string& modelName);
    [[nodiscard]] static Texture LoadTexture(const std::string& texturePath);
    // NOTE(v.matushkin): Format extension'.shader' is added implicitly to shaderName to find it on disk, but stored in m_shaders without it
    [[nodiscard]] static Shader  LoadShader(const std::string& shaderName);

private:
    static inline std::string m_assetDir;
    static inline std::string m_modelDir;
    static inline std::string m_glShaderDir;
    static inline std::string m_vkShaderDir;
    static inline std::string m_dxShaderDir;

    static inline Models   m_models;   // NOTE(v.matushkin): Model path is relative to 'assets/models/'
    static inline Textures m_textures;
    static inline Shaders  m_shaders;  // NOTE(v.matushkin): Shader path is relative to 'assets/[dx|gl|vk]/'
};

} // namespace snv
