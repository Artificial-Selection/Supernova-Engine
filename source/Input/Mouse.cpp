#include <Input/Mouse.hpp>


namespace snv::Input
{

InputAction Mouse::GetButton(MouseButton key)
{
    return m_buttons[static_cast<i32>(key)];
}

// TODO(v.matushkin): Same shit as for Keyboard::IsKeyPressed()
//   Although seems like mouse doesn't recieve InputAction::Repeat anyway
bool Mouse::IsButtonPressed(MouseButton key)
{
    return GetButton(key) != InputAction::Release;
}

bool Mouse::IsButtonReleased(MouseButton key)
{
    return GetButton(key) == InputAction::Release;
}


void Mouse::ButtonCallback(i32 button, i32 action, i32 mods)
{
    m_buttons[button] = static_cast<InputAction>(action);
}

void Mouse::PositionCallback(f64 xpos, f64 ypos)
{
    m_mousePosition = glm::dvec2(xpos, ypos);
}

void Mouse::WheelCallback(f64 yoffset)
{
    m_mouseWheelOffsetY = yoffset;
}

} // namespace snv::Input
