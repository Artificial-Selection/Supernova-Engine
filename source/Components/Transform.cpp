#include <Components/Transform.hpp>

#include <glm/gtx/quaternion.hpp>


namespace snv
{

Transform::Transform() noexcept
    : m_position(0.0f)
    , m_scale(1.0f)
    , m_rotation(glm::identity<glm::quat>())
    , m_transform(glm::identity<glm::mat4>())
    , m_dirty(false)
{}


const glm::mat4x4& Transform::GetMatrix() const
{
    if (m_dirty)
    {
        auto R = glm::toMat4(m_rotation);
        auto T = glm::translate(R, m_position);
        m_transform = glm::scale(T, m_scale);
        m_dirty = false;
    }

    return m_transform;
}

glm::vec3 Transform::GetRotationEuler() const
{
    return glm::eulerAngles(m_rotation);
}

glm::vec3 Transform::GetRotationEulerDeg() const
{
    return glm::degrees(GetRotationEuler());
}


void Transform::SetPosition(const glm::vec3& position)
{
    m_position = position;
    m_dirty = true;
}

void Transform::SetPosition(f32 x, f32 y, f32 z)
{
    SetPosition(glm::vec3(x, y, z));
}

void Transform::SetScale(const glm::vec3& scale)
{
    m_scale = scale;
    m_dirty = true;
}

void Transform::SetScale(f32 x, f32 y, f32 z)
{
    SetScale(glm::vec3(x, y, z));
}

void Transform::SetScale(f32 scale)
{
    SetScale(glm::vec3(scale));
}

void Transform::SetRotation(const glm::quat& rotation)
{
    m_rotation = rotation;
    m_dirty = true;
}

void Transform::SetRotation(const glm::vec3& degrees)
{
    SetRotation(glm::quat(glm::radians(degrees)));
}

void Transform::SetRotation(f32 xDegrees, f32 yDegrees, f32 zDegrees)
{
    SetRotation(glm::vec3(xDegrees, yDegrees, zDegrees));
}


void Transform::Translate(const glm::vec3& translation)
{
    m_position += translation;
    m_dirty = true;
}

void Transform::Translate(f32 x, f32 y, f32 z)
{
    Translate(glm::vec3(x, y, z));
}

void Transform::Scale(const glm::vec3& scale)
{
    m_scale *= scale;
    m_dirty = true;
}

void Transform::Scale(f32 x, f32 y, f32 z)
{
    Scale(glm::vec3(x, y, z));
}

void Transform::Scale(f32 scale)
{
    m_scale *= scale;
    m_dirty = true;
}

void Transform::Rotate(const glm::quat& rotation)
{
    m_rotation *= rotation;
    m_dirty = true;
}

void Transform::Rotate(const glm::vec3& degrees)
{
    Rotate(glm::quat(glm::radians(degrees)));
}

void Transform::Rotate(f32 xDegrees, f32 yDegrees, f32 zDegrees)
{
    Rotate(glm::vec3(xDegrees, yDegrees, zDegrees));
}

} // namespace snv
