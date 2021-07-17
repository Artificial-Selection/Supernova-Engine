#include <Core/Window.hpp>
#include <Core/Log.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>


// NOTE(v.matushkin): I'm initializing glad here only because its header needs to be included
//  before glfw and because I'm not sure where I should do it, if not here(GLBackend ?)
//  Leave this shit to the future me


void GlfwErrorCallback(i32 what_is_this, const char* error)
{
    LOG_ERROR("GLFW: {}", error);
}


namespace snv
{

static_assert(static_cast<i32>(KeyStatus::Release) == GLFW_RELEASE);
static_assert(static_cast<i32>(KeyStatus::Press)   == GLFW_PRESS);

static_assert(static_cast<i32>(KeyCode::A)      == GLFW_KEY_A);
static_assert(static_cast<i32>(KeyCode::D)      == GLFW_KEY_D);
static_assert(static_cast<i32>(KeyCode::E)      == GLFW_KEY_E);
static_assert(static_cast<i32>(KeyCode::Q)      == GLFW_KEY_Q);
static_assert(static_cast<i32>(KeyCode::S)      == GLFW_KEY_S);
static_assert(static_cast<i32>(KeyCode::W)      == GLFW_KEY_W);
static_assert(static_cast<i32>(KeyCode::X)      == GLFW_KEY_X);
static_assert(static_cast<i32>(KeyCode::Z)      == GLFW_KEY_Z);
static_assert(static_cast<i32>(KeyCode::Escape) == GLFW_KEY_ESCAPE);


Window::Window(i32 width, i32 height, const char* title)
{
    glfwSetErrorCallback(GlfwErrorCallback);
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
#ifdef SNV_ENABLE_DEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

    m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (m_window == nullptr)
    {
        LOG_CRITICAL("Failed to create GLFW window");
        // TODO: Assert
        //return -1;
    }

    glfwMakeContextCurrent(m_window);

    if (gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) == 0)
    {
        LOG_CRITICAL("Failed to initialize GLAD");
        // TODO: Assert
        //return -1;
    }
}

Window::~Window()
{
    glfwTerminate();
}


KeyStatus Window::GetKey(KeyCode key) const
{
    return static_cast<KeyStatus>(glfwGetKey(m_window, static_cast<i32>(key)));
}

bool Window::IsShouldBeClosed() const
{
    return glfwWindowShouldClose(m_window) != 0;
}


void Window::Close() const
{
    // NOTE(v.matushkin): Check IsOpen?
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

} // namespace snv
