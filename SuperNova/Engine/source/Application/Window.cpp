#include <Engine/Application/Window.hpp>

#include <Engine/Core/Assert.hpp>
#include <Engine/Input/Cursor.hpp>
#include <Engine/Renderer/RenderTypes.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#ifdef SNV_PLATFORM_WINDOWS
    #define GLFW_EXPOSE_NATIVE_WIN32
    #include <GLFW/glfw3native.h>
    #undef GLFW_EXPOSE_NATIVE_WIN32
#endif

// NOTE(v.matushkin): I'm initializing glad here only because its header needs to be included
//  before glfw and because I'm not sure where I should do it, if not here(GLBackend ?)
//  Leave this shit to the future me


const i32 glfw_CursorMode[] = {
    GLFW_CURSOR_NORMAL,  // Cursor::Normal
    GLFW_CURSOR_HIDDEN,  // Cursor::Hidden
    GLFW_CURSOR_DISABLED // Cursor::Locked
};


#ifdef SNV_ENABLE_DEBUG
void GlfwErrorCallback(i32 what_is_this, const char* error)
{
    LOG_ERROR("GLFW: {}", error);
}
#endif


namespace snv
{

// TODO(v.matushkin): I shouldn't pass GraphicsApi to the Window
//  This whole shit about OpenGL should be in GLBackend, not here
//  But custom OpenGL Context creation for win/linux/mac seems harder than for Vulkan,
//  so leave it like that for now

void Window::Init(i32 width, i32 height, const char* title, GraphicsApi graphicsApi)
{
    m_width  = width;
    m_height = height;

#ifdef SNV_ENABLE_DEBUG
    glfwSetErrorCallback(GlfwErrorCallback);
#endif
    glfwInit();
    if (graphicsApi == GraphicsApi::OpenGL)
    {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
#ifdef SNV_GPU_API_DEBUG_ENABLED
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif
    }
    else
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    }

    m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    SNV_ASSERT(m_window != nullptr, "Failed to create GLFW window");

    if (graphicsApi == GraphicsApi::OpenGL)
    {
        glfwMakeContextCurrent(m_window);

        const auto glad_dont_know_what = gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
        SNV_ASSERT(glad_dont_know_what != 0, "Failed to initialize GLAD");
    }

    // NOTE(v.matushkin): Do I need to set callbacks here or only when Set*Callback methods are called?
    // NOTE(v.matushkin): From GLFW doc:
    //  When a window loses input focus, it will generate synthetic key release events for all pressed keys.
    //  You can tell these events from user-generated events by the fact that the synthetic ones are generated
    //  after the focus loss event has been processed, i.e. after the window focus callback has been called.
    glfwSetKeyCallback(m_window, Window::GLFWKeyCallback);
    glfwSetMouseButtonCallback(m_window, Window::GLFWMouseButtonCallback);
    glfwSetCursorPosCallback(m_window, Window::GLFWMousePositionCallback);
    glfwSetScrollCallback(m_window, Window::GLFWMouseWheelCallback);
}

void Window::Shutdown()
{
    glfwTerminate();
}


#ifdef SNV_PLATFORM_WINDOWS
HWND Window::GetWin32Window()
{
    return glfwGetWin32Window(m_window);
}
#endif // SNV_PLATFORM_WINDOWS


bool Window::IsShouldBeClosed()
{
    return glfwWindowShouldClose(m_window) != 0;
}


void Window::SetKeyCallback(KeyCallback keyCallback)
{
    m_keyCallback = keyCallback;
}

void Window::SetMouseButtonCallback(MouseButtonCallback mouseButtonCallback)
{
    m_mouseButtonCallback = mouseButtonCallback;
}

void Window::SetMousePositionCallback(MousePositionCallback mousePositionCallback)
{
    m_mousePositionCallback = mousePositionCallback;
}

void Window::SetMouseWheelCallback(MouseWheelCallback mouseWheelCallback)
{
    m_mouseWheelCallback = mouseWheelCallback;
}


void Window::SetCursorMode(Input::CursorMode cursorMode)
{
    const i32 glfwCursorMode = glfw_CursorMode[static_cast<ui8>(cursorMode)];
    glfwSetInputMode(m_window, GLFW_CURSOR, glfwCursorMode);
}


void Window::Close()
{
    // NOTE(v.matushkin): Check IsOpen() ?
    glfwSetWindowShouldClose(m_window, GLFW_TRUE);
}

void Window::PollEvents()
{
    glfwPollEvents();
}

void Window::SwapBuffers()
{
    glfwSwapBuffers(m_window);
}


void Window::GLFWKeyCallback(GLFWwindow* glfwWindow, i32 key, i32 scancode, i32 action, i32 mods)
{
    SNV_ASSERT(key != GLFW_KEY_UNKNOWN, "GLFW_KEY_UNKNOWN is not handled");

    /*const char* actionString;
    if (action == GLFW_REPEAT)
        actionString = "REPEAT";
    else if (action == GLFW_PRESS)
        actionString = "PRESS";
    else
        actionString = "RELEASE";
    LOG_INFO("[Window::GLFWKeyCallback] Key: {} | Action: {}", key, actionString);*/

    SNV_ASSERT(m_keyCallback != nullptr, "KeyCallback was not set");
    m_keyCallback(key, scancode, action, mods);
}

void Window::GLFWMouseButtonCallback(GLFWwindow* glfwWindow, i32 button, i32 action, i32 mods)
{
    /*const char* actionString;
    if (action == GLFW_REPEAT)
        actionString = "REPEAT";
    else if (action == GLFW_PRESS)
        actionString = "PRESS";
    else
        actionString = "RELEASE";
    LOG_INFO("[Window::GLFWMouseButtonCallback] Button: {} | Action: {}", button, actionString);*/

    SNV_ASSERT(m_mouseButtonCallback != nullptr, "MouseButtonCallback was not set");
    m_mouseButtonCallback(button, action, mods);
}

void Window::GLFWMousePositionCallback(GLFWwindow* glfwWindow, f64 xpos, f64 ypos)
{
    //LOG_INFO("[Window::GLFWCursorPositionCallback] xpos: {} | ypos: {}", xpos, ypos);

    SNV_ASSERT(m_mousePositionCallback != nullptr, "MousePositionCallback was not set");
    m_mousePositionCallback(xpos, ypos);
}

void Window::GLFWMouseWheelCallback(GLFWwindow* glfwWindow, f64 xoffset, f64 yoffset)
{
    //LOG_INFO("[Window::GLFWMouseWheelCallback] xoffset: {} | yoffset: {}", xoffset, yoffset);

    SNV_ASSERT(m_mouseWheelCallback != nullptr, "MouseWheelCallback was not set");
    m_mouseWheelCallback(yoffset);
}

} // namespace snv
