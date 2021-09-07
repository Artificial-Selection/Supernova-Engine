#pragma once

#include <Engine/Input/Input.hpp>
#include <Engine/Input/KeyboardKey.hpp>


namespace snv::Input
{

class Keyboard
{
public:
    // NOTE(v.matushkin): The new Unity.InputSystem looks interesting, but it's far more complicated than I need rn
    // NOTE(v.matushkin): May be instead of GetKey() there should be something like:
    //  - bool IsPressed /GetPressed /GetDown
    //  - bool IsReleased/GetReleased/GetUp
    [[nodiscard]] static InputAction GetKey(KeyboardKey key);
    [[nodiscard]] static bool IsKeyPressed(KeyboardKey key);
    [[nodiscard]] static bool IsKeyReleased(KeyboardKey key);

    // TODO: This should not be public, only visible to some 'friend' class or something like this
    static void KeyCallback(i32 key, i32 scancode, i32 action, i32 mods);

private:
    static inline InputAction m_keys[349];
};

}
