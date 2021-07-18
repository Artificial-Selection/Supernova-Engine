#include <Core/Window.hpp>
#include <Core/Assert.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>


// NOTE(v.matushkin): I'm initializing glad here only because its header needs to be included
//  before glfw and because I'm not sure where I should do it, if not here(GLBackend ?)
//  Leave this shit to the future me


#ifdef SNV_ENABLE_DEBUG
void GlfwErrorCallback(i32 what_is_this, const char* error)
{
    LOG_ERROR("GLFW: {}", error);
}
#endif


namespace snv
{

Window::Window(i32 width, i32 height, const char* title)
{
#ifdef SNV_ENABLE_DEBUG
    glfwSetErrorCallback(GlfwErrorCallback);
#endif
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
#ifdef SNV_ENABLE_DEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

    m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    SNV_ASSERT(m_window != nullptr, "Failed to create GLFW window");

    glfwMakeContextCurrent(m_window);

    const auto glad_dont_know_what = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    SNV_ASSERT(glad_dont_know_what != 0, "Failed to initialize GLAD");

    glfwSetWindowUserPointer(m_window, this);

    // NOTE(v.matushkin): Do I need to set callbacks here or only when Set*Callback methods are called?
    // When a window loses input focus, it will generate synthetic key release events for all pressed keys.
    //  You can tell these events from user-generated events by the fact that the synthetic ones are generated
    //  after the focus loss event has been processed, i.e. after the window focus callback has been called.
    glfwSetKeyCallback(m_window, Window::GLFWKeyCallback);
    glfwSetMouseButtonCallback(m_window, Window::GLFWMouseButtonCallback);
}

Window::~Window()
{
    glfwTerminate();
}


bool Window::IsShouldBeClosed() const
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


void Window::Close() const
{
    // NOTE(v.matushkin): Check IsOpen() ?
    glfwSetWindowShouldClose(m_window, GLFW_TRUE);
}

void Window::PollEvents() const
{
    glfwPollEvents();
}

void Window::SwapBuffers() const
{
    glfwSwapBuffers(m_window);
}


void Window::GLFWKeyCallback(GLFWwindow* glfwWindow, i32 key, i32 scancode, i32 action, i32 mods)
{
    SNV_ASSERT(key != GLFW_KEY_UNKNOWN, "GLFW_KEY_UNKNOWN is not handled");

    const char* actionString;
    if (action == GLFW_REPEAT)
        actionString = "REPEAT";
    else if (action == GLFW_PRESS)
        actionString = "PRESS";
    else
        actionString = "RELEASE";
    LOG_INFO("[Window::GLFWKeyCallback] Key: {} | Action: {}", key, actionString);

    const auto window = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));

    SNV_ASSERT(window->m_keyCallback != nullptr, "KeyCallback was not set");
    window->m_keyCallback(key, scancode, action, mods);
}

void Window::GLFWMouseButtonCallback(GLFWwindow* glfwWindow, i32 button, i32 action, i32 mods)
{
    const char* actionString;
    if (action == GLFW_REPEAT)
        actionString = "REPEAT";
    else if (action == GLFW_PRESS)
        actionString = "PRESS";
    else
        actionString = "RELEASE";
    LOG_INFO("[Window::GLFWMouseButtonCallback] Button: {} | Action: {}", button, actionString);

    const auto window = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));

    SNV_ASSERT(window->m_mouseButtonCallback != nullptr, "MouseButtonCallback was not set");
    window->m_mouseButtonCallback(button, action, mods);
}

} // namespace snv
