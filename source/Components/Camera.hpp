#pragma once

#include <Core/Core.hpp>
#include <Components/Component.hpp>

#include <glm/ext/matrix_float4x4.hpp>


// TODO(v.matushkin): Add ortho camera support


namespace snv
{

class Camera final : public BaseComponent
{
public:
    Camera() noexcept;
    Camera(f32 fieldOfView, f32 aspectRatio, f32 nearClipPlane, f32 farClipPlane) noexcept;

    [[nodiscard]] const glm::mat4x4& GetProjectionMatrix() const;
    [[nodiscard]] f32 GetFieldOfView()   const { return m_fieldOfView; }
    [[nodiscard]] f32 GetAspectRatio()   const { return m_aspectRatio; }
    [[nodiscard]] f32 GetNearClipPlane() const { return m_nearClipPlane; }
    [[nodiscard]] f32 GetFarClipPlane()  const { return m_farClipPlane; }

    void SetFieldOfView(f32 fieldOfView);
    void SetAspectRatio(f32 aspectRatio);
    void SetNearClipPlane(f32 nearClipPlane);
    void SetFarClipPlane(f32 farClipPlane);

private:
    f32 m_fieldOfView;
    f32 m_aspectRatio;
    f32 m_nearClipPlane;
    f32 m_farClipPlane;

    mutable glm::mat4x4 m_projection;
    mutable bool        m_dirty;
};

} // namespace snv
