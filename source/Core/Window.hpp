#pragma once

#include <Core/Core.hpp>

#include <functional>


class GLFWwindow;


namespace snv
{

namespace Input
{
    enum class CursorMode : ui8;
}


class Window
{
public:
    using KeyCallback           = std::function<void(i32 key, i32 scancode, i32 action, i32 mods)>;
    using MouseButtonCallback   = std::function<void(i32 button, i32 action, i32 mods)>;
    using MousePositionCallback = std::function<void(f64 xpos, f64 ypos)>;
    using MouseWheelCallback    = std::function<void(f64 yoffset)>;

public:
    static void Init(i32 width, i32 height, const char* title);
    static void Shutdown();

    [[nodiscard]] static bool IsShouldBeClosed() ;
    // TODO(v.matushkin): Need to be consistent with Get*/Set* methods definitions, always define in .cpp or .hpp ?
    static void SetKeyCallback(KeyCallback keyCallback);
    static void SetMouseButtonCallback(MouseButtonCallback mouseButtonCallback);
    static void SetMousePositionCallback(MousePositionCallback mousePositionCallback);
    static void SetMouseWheelCallback(MouseWheelCallback mouseWheelCallback);

    // TODO(v.matushkin): only accessible by snv::Input::Cursor?
    static void SetCursorMode(Input::CursorMode cursorMode);

    static void Close();
    static void PollEvents();
    static void SwapBuffers();

private:
    static void GLFWKeyCallback(GLFWwindow* glfwWindow, i32 key, i32 scancode, i32 action, i32 mods);
    static void GLFWMouseButtonCallback(GLFWwindow* glfwWindow, i32 button, i32 action, i32 mods);
    static void GLFWMousePositionCallback(GLFWwindow* glfwWindow, f64 xpos, f64 ypos);
    static void GLFWMouseWheelCallback(GLFWwindow* glfwWindow, f64 xoffset, f64 yoffset);

private:
    static inline GLFWwindow*           m_window;

    static inline KeyCallback           m_keyCallback;
    static inline MouseButtonCallback   m_mouseButtonCallback;
    static inline MousePositionCallback m_mousePositionCallback;
    static inline MouseWheelCallback    m_mouseWheelCallback;
};

} // namespace snv
