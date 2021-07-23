#pragma once

#include <Core/Core.hpp>

#include <functional>


class GLFWwindow;


namespace snv
{

class Window
{
public:
    using KeyCallback           = std::function<void(i32 key, i32 scancode, i32 action, i32 mods)>;
    using MouseButtonCallback   = std::function<void(i32 button, i32 action, i32 mods)>;
    using MousePositionCallback = std::function<void(f64 xpos, f64 ypos)>;

public:
    Window(i32 width, i32 height, const char* title);
    ~Window();

    [[nodiscard]] bool IsShouldBeClosed() const;
    // TODO(v.matushkin): Need to be consistent with Get*/Set* methods definitions, always define in .cpp or .hpp ?
    void SetKeyCallback(KeyCallback keyCallback);
    void SetMouseButtonCallback(MouseButtonCallback mouseButtonCallback);
    void SetMousePositionCallback(MousePositionCallback mousePositionCallback);

    void Close() const;
    void PollEvents() const;
    void SwapBuffers() const;

private:
    static void GLFWKeyCallback(GLFWwindow* glfwWindow, i32 key, i32 scancode, i32 action, i32 mods);
    static void GLFWMouseButtonCallback(GLFWwindow* glfwWindow, i32 button, i32 action, i32 mods);
    static void GLFWMousePositionCallback(GLFWwindow* glfwWindow, f64 xpos, f64 ypos);

private:
    GLFWwindow*           m_window;

    KeyCallback           m_keyCallback;
    MouseButtonCallback   m_mouseButtonCallback;
    MousePositionCallback m_mousePositionCallback;
};

} // namespace snv
