#include <Engine/Components/Camera.hpp>

#include <glm/gtx/transform.hpp>


namespace snv
{

Camera::Camera(GameObject* gameObject) noexcept
    : Camera(gameObject, 90.0f, 16.0f / 9.0f, 0.1f, 1000.0f)
{}

Camera::Camera(GameObject* gameObject, f32 fieldOfView, f32 aspectRatio, f32 nearClipPlane, f32 farClipPlane) noexcept
    : BaseComponent(gameObject)
    , m_fieldOfView(glm::radians(fieldOfView))
    , m_aspectRatio(aspectRatio)
    , m_nearClipPlane(nearClipPlane)
    , m_farClipPlane(farClipPlane)
    , m_projection(glm::perspective(m_fieldOfView, m_aspectRatio, m_nearClipPlane, m_farClipPlane))
    , m_dirty(false)
{}


const glm::mat4x4& Camera::GetProjectionMatrix() const
{
    if (m_dirty)
    {
        m_projection = glm::perspective(m_fieldOfView, m_aspectRatio, m_nearClipPlane, m_farClipPlane);
        m_dirty = false;
    }

    return m_projection;
}


void Camera::SetFieldOfView(f32 fieldOfView)
{
    m_fieldOfView = glm::radians(fieldOfView);
    m_dirty = true;
}

void Camera::SetAspectRatio(f32 aspectRatio)
{
    m_aspectRatio = aspectRatio;
    m_dirty = true;
}

void Camera::SetNearClipPlane(f32 nearClipPlane)
{
    m_nearClipPlane = nearClipPlane;
    m_dirty = true;
}

void Camera::SetFarClipPlane(f32 farClipPlane)
{
    m_farClipPlane = farClipPlane;
    m_dirty = true;
}

} // namespace snv
