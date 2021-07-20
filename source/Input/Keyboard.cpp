#include <Input/Keyboard.hpp>


namespace snv::Input
{

InputAction Keyboard::m_keys[349];
Keyboard::KeyEventCallback Keyboard::m_keyEventListener;


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
    const auto keyboardKey = static_cast<KeyboardKey>(key);
    const auto inputAction = static_cast<InputAction>(action);
    m_keys[key] = inputAction;
    m_keyEventListener({ keyboardKey , inputAction });
}

} // namespace snv::Input
