#pragma once

#include <Core/Core.hpp>
#include <Components/Component.hpp>


namespace snv
{

class CameraController final : public BaseComponent
{
public:
    CameraController();

    void OnUpdate();

private:
    void ProcessMouseInput();

private:
    bool m_init = false;

    f64 m_mousePositionX = 0;
    f64 m_mousePositionY = 0;

    f32 m_yaw;
    f32 m_pitch;
};

} // namespace snv
