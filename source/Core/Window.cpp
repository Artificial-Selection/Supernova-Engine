//
// Created by Devilast on 7/10/2021.
//

#include "Window.hpp"
#include "Log.hpp"
#include "GLFW/glfw3.h"

GLFWwindow* Window::m_windowInternal = nullptr;
bool Window::m_shouldBeClosed = false;

void Window::SetKeyPressedCallback(GLFWkeyfun keyCallback)
{
    glfwSetKeyCallback(m_windowInternal, keyCallback);
}

void Window::PollEvents()
{
    glfwPollEvents();
}

void Window::Initialize()
{
    LOG_TRACE("SuperNova-Engine Init");

    glfwSetErrorCallback(GlfwErrorCallback);
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, false);
#ifdef SNV_ENABLE_DEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif

    m_windowInternal = glfwCreateWindow(DefaultWindowWidth, DefaultWindowHeight, "SuperNova-Engine", nullptr, nullptr);
    HALT(m_windowInternal != nullptr, "Failed to create GLFW window");

    glfwMakeContextCurrent(m_windowInternal);
}

void Window::GlfwErrorCallback(int what_is_this, const char* error)
{
    LOG_ERROR("GLFW: {}", error);
}

bool Window::IsShouldBeClosed()
{
    return m_shouldBeClosed;
}

void Window::SwapBuffers()
{
    glfwSwapBuffers(m_windowInternal);
}

void Window::Terminate()
{
    glfwTerminate();
}

void Window::GetProcAddress(const char* procName)
{
    glfwGetProcAddress(procName);
}
