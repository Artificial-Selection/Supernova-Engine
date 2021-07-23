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
// TODO(v.matushkin): Need to have scroll delta, but idk how to do it 

class Mouse
{
public:
    [[nodiscard]] static InputAction GetButton(MouseButton key);
    [[nodiscard]] static bool IsButtonPressed(MouseButton key);
    [[nodiscard]] static bool IsButtonReleased(MouseButton key);

    // NOTE(v.matushkin): Is there any use of storing mouse position as 'f64' ?
    [[nodiscard]] static glm::dvec2 GetMousePosition() { return m_mousePosition; }
    [[nodiscard]] static f32        GetWheelOffsetY()  { return m_mouseWheelOffsetY; }

    // TODO: This should not be public, only visible to some 'friend App' class or something like this
    static void ButtonCallback(i32 button, i32 action, i32 mods);
    static void PositionCallback(f64 xpos, f64 ypos);
    static void WheelCallback(f64 yoffset);

private:
    static inline InputAction m_buttons[8];
    static inline glm::dvec2  m_mousePosition;
    static inline f32         m_mouseWheelOffsetY; // NOTE(v.matushkin): Not a good name?
};

} // namespace snv::Input
