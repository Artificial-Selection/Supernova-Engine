#pragma once

#include <memory>


namespace snv
{

class Texture;


class Material
{
public:
    Material(std::shared_ptr<Texture> diffuseTexture);

    [[nodiscard]] const std::shared_ptr<Texture>& GetDiffuseTexture() const { return m_diffuseTexture; }

private:
    std::shared_ptr<Texture> m_diffuseTexture;
};

} // namespace snv
