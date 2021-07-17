#pragma once

#include <Core/Core.hpp>

#include <functional>


class GLFWwindow;


namespace snv
{

class Window
{
public:
    using KeyCallback = std::function<void(i32 key, i32 scancode, i32 action, i32 mods)>;

public:
    Window(i32 width, i32 height, const char* title);
    ~Window();

    [[nodiscard]] bool IsShouldBeClosed() const;

    void SetKeyCallback(KeyCallback keyCallback);

    void Close() const;
    void PollEvents() const;
    void SwapBuffers() const;

private:
    static void GLFWKeyCallback(GLFWwindow* glfwWindow, i32 key, i32 scancode, i32 action, i32 mods);

private:
    GLFWwindow* m_window;

    KeyCallback m_keyCallback;
};

} // namespace snv
