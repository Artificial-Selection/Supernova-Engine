#include <Input/Keyboard.hpp>


namespace snv::Input
{

InputAction Keyboard::m_keys[349];


InputAction Keyboard::GetKey(KeyboardKey key)
{
    return m_keys[static_cast<i32>(key)];
}

// TODO(v.matushkin): For now, return true on KeyAction::Press and KeyAction::Repat
//   But probably should not be like that
bool Keyboard::IsKeyPressed(KeyboardKey key)
{
    return GetKey(key) != InputAction::Release;
}

bool Keyboard::IsKeyReleased(KeyboardKey key)
{
    return GetKey(key) == InputAction::Release;
}


void Keyboard::KeyCallback(i32 key, i32 scancode, i32 action, i32 mods)
{
    m_keys[key] = static_cast<InputAction>(action);
}

} // namespace snv::Input
