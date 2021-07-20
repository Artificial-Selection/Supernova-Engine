#pragma once

#include <memory>
#include <string>
#include <string_view>


namespace snv
{

class Texture;


class Material
{
public:
    Material(std::string_view materialName);

    [[nodiscard]] const std::shared_ptr<Texture>& GetBaseColorMap() const { return m_baseColorMap; }
    [[nodiscard]] const std::shared_ptr<Texture>& GetNormalMap()    const { return m_normalMap; }

    void SetBaseColorMap(std::shared_ptr<Texture> baseColorMap) { m_baseColorMap = baseColorMap; }
    void SetNormalMap   (std::shared_ptr<Texture> normalMap)    { m_normalMap    = normalMap; }

private:
    std::string              m_materialName;

    std::shared_ptr<Texture> m_baseColorMap;
    std::shared_ptr<Texture> m_normalMap;
};

} // namespace snv
