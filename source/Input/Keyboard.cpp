#include <Input/Keyboard.hpp>


namespace snv::Input
{

KeyAction Keyboard::m_keys[349];


KeyAction Keyboard::GetKey(KeyboardKey key)
{
    return m_keys[static_cast<i32>(key)];
}

// TODO(v.matushkin): For now, return true on KeyAction::Press and KeyAction::Repat
//   But probably should not be like that
bool Keyboard::IsKeyPressed(KeyboardKey key)
{
    return GetKey(key) != KeyAction::Release;
}

bool Keyboard::IsKeyReleased(KeyboardKey key)
{
    return GetKey(key) == KeyAction::Release;
}


void Keyboard::KeyCallback(i32 key, i32 scancode, i32 action, i32 mods)
{
    m_keys[key] = static_cast<KeyAction>(action);
}

} // namespace snv::Input
