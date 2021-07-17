#pragma once

#include <Core/Core.hpp>


class GLFWwindow;


namespace snv
{

enum class KeyStatus : i32
{
    Release = 0,
    Press   = 1
};

enum class KeyCode : i32
{
    A      = 65,
    D      = 68,
    E      = 69,
    Q      = 81,
    S      = 83,
    W      = 87,
    X      = 88,
    Z      = 90,
    Escape = 256
};


class Window
{
public:
    Window(i32 width, i32 height, const char* title);
    ~Window();

    // NOTE(v.matushkin): The new Unity.InputSystem looks interesting, but it's far more complicated than I need rn
    // NOTE(v.matushkin): May be instead of GetKey() there should be something like:
    //  - bool IsPressed /GetPressed /GetDown
    //  - bool IsReleased/GetReleased/GetUp
    [[nodiscard]] KeyStatus GetKey(KeyCode key) const;
    [[nodiscard]] bool IsShouldBeClosed() const;

    void Close() const;
    void PollEvents() const;
    void SwapBuffers() const;

private:
    GLFWwindow* m_window;
};

} // namespace snv
