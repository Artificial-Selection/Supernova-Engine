#include <Assets/Material.hpp>
#include <Renderer/Renderer.hpp>


namespace snv
{

Material::Material(const TextureDescriptor& textureDescriptor, const ui8* data)
{
    m_textureHandle = snv::Renderer::CreateTexture(textureDescriptor, data);
}

} // namespace snv
