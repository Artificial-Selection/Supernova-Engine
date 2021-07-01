#include <Components/Transform.hpp>


namespace snv
{

Transform::Transform() noexcept
    : m_transform(glm::mat4x4(1.0f))
{}

const glm::mat4x4& Transform::GetTransform() const noexcept
{
    return m_transform;
}

} // namespace snv
