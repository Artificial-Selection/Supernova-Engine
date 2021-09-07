#pragma once

#include <Engine/Core/Core.hpp>
#include <Engine/Components/Component.hpp>


namespace snv
{

class Transform;


class CameraController final : public BaseComponent
{
public:
    CameraController(GameObject* gameObject, f32 movementSpeed, f32 movementBoost);

    void SetMovementSpeed(f32 movementSpeed);
    void SetMovementBoost(f32 movementBoost);

    void OnUpdate();

private:
    void ProcessMouseInput();

private:
    Transform* m_cameraTransform;

    f64 m_mousePositionX = 0;
    f64 m_mousePositionY = 0;

    f32 m_yaw;
    f32 m_pitch;

    f32 m_movementSpeed;
    f32 m_movementBoost;
};

} // namespace snv
