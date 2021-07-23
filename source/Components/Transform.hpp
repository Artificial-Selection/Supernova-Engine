#pragma once

#include <Core/Core.hpp>
#include <Components/Component.hpp>

#include <glm/ext/vector_float3.hpp>
#include <glm/ext/quaternion_common.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/matrix_float4x4.hpp>


namespace snv
{

class Transform final : public BaseComponent
{
public:
    Transform() noexcept;

    [[nodiscard]] const glm::mat4x4& GetMatrix() const;
    [[nodiscard]] const glm::vec3& GetPosition() const { return m_position; }
    [[nodiscard]] const glm::vec3& GetScale()    const { return m_scale; }
    [[nodiscard]] const glm::quat& GetRotation() const { return m_rotation; }
    [[nodiscard]] glm::vec3 GetRotationEuler() const;
    [[nodiscard]] glm::vec3 GetRotationEulerDeg() const;

    void SetPosition(const glm::vec3& position);
    void SetPosition(f32 x, f32 y, f32 z);
    void SetScale(const glm::vec3& scale);
    void SetScale(f32 x, f32 y, f32 z);
    void SetScale(f32 scale);
    void SetRotation(const glm::quat& rotation);
    void SetRotation(const glm::vec3& degrees);
    void SetRotation(f32 xDegrees, f32 yDegrees, f32 zDegrees);

    void Translate(const glm::vec3& translation);
    void Translate(f32 x, f32 y, f32 z);
    void Scale(const glm::vec3& scale);
    void Scale(f32 x, f32 y, f32 z);
    void Scale(f32 scale);
    void Rotate(const glm::quat& rotation);
    // TODO(v.matushkin): Most likely this Rotate methods doesn't work and I need to use glm::angleAxis
    void Rotate(const glm::vec3& degrees);
    void Rotate(f32 xDegrees, f32 yDegrees, f32 zDegrees);

private:
    glm::vec3 m_position;
    glm::vec3 m_scale;
    glm::quat m_rotation;

    mutable glm::mat4x4 m_transform;
    mutable bool        m_dirty;
};

} // namespace snv
