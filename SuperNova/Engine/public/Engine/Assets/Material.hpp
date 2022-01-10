#pragma once

#include <memory>
#include <string>


namespace snv
{

// NOTE(v.matushkin): May be it's not a good idea to forward these
class Texture;
class Shader;


class Material
{
public:
    Material(std::shared_ptr<Shader> shader);

    [[nodiscard]] const std::shared_ptr<Shader>& GetShader() const { return m_shader; }

    [[nodiscard]] const std::shared_ptr<Texture>& GetBaseColorMap() const { return m_baseColorMap; }
    [[nodiscard]] const std::shared_ptr<Texture>& GetNormalMap()    const { return m_normalMap; }

    void SetName(std::string name);

    void SetBaseColorMap(std::shared_ptr<Texture> baseColorMap) { m_baseColorMap = baseColorMap; }
    void SetNormalMap   (std::shared_ptr<Texture> normalMap)    { m_normalMap    = normalMap; }

private:
    std::string              m_materialName;
    std::shared_ptr<Shader>  m_shader;

    std::shared_ptr<Texture> m_baseColorMap;
    std::shared_ptr<Texture> m_normalMap;
};

} // namespace snv
