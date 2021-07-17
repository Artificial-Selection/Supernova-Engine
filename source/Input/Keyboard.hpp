#pragma once

#include <Input/KeyboardKey.hpp>


namespace snv::Input
{

enum class KeyAction : i8
{
    Release = 0,
    Press   = 1,
    Repeat  = 2
};


class Keyboard
{
public:
    // NOTE(v.matushkin): The new Unity.InputSystem looks interesting, but it's far more complicated than I need rn
    // NOTE(v.matushkin): May be instead of GetKey() there should be something like:
    //  - bool IsPressed /GetPressed /GetDown
    //  - bool IsReleased/GetReleased/GetUp
    [[nodiscard]] static KeyAction GetKey(KeyboardKey key);
    [[nodiscard]] static bool IsKeyPressed(KeyboardKey key);
    [[nodiscard]] static bool IsKeyReleased(KeyboardKey key);


    // TODO: This should not be public, only visible to some 'friend' class
    static void KeyCallback(i32 key, i32 scancode, i32 action, i32 mods);

private:
    static KeyAction m_keys[349];
};

}
