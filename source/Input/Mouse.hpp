#pragma once

#include <Input/Input.hpp>

#include <glm/ext/vector_double2.hpp>


namespace snv::Input
{

// TODO(v.matushkin): There is 8 mouse buttons in GLFW, but I can't test them and rn they are useless anyway
enum class MouseButton
{
    Left   = 0,
    Right  = 1,
    Middle = 2
};


// TODO(v.matushkin): Handle Mouse position/scroll change, cursor leave/enter
class Mouse
{
public:
    [[nodiscard]] static InputAction GetButton(MouseButton key);
    [[nodiscard]] static bool IsButtonPressed(MouseButton key);
    [[nodiscard]] static bool IsButtonReleased(MouseButton key);

    // NOTE(v.matushkin): Is there any use of storing mouse position as 'f64' ?
    [[nodiscard]] static glm::dvec2 GetMousePosition() { return m_mousePosition; }

    // TODO: This should not be public, only visible to some 'friend App' class or something like this
    static void ButtonCallback(i32 button, i32 action, i32 mods);
    static void PositionCallback(f64 xpos, f64 ypos);

private:
    static inline InputAction m_buttons[8];
    static inline glm::dvec2 m_mousePosition;
};

} // namespace snv::Input
