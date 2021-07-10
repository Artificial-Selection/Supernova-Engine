//
// Created by Devilast on 7/10/2021.
//
#pragma once

class GLFWwindow;

typedef void(* GLFWkeyfun)(GLFWwindow*, int, int, int, int);

class Window
{
public:
    static const int DefaultWindowWidth = 640;
    static const int DefaultWindowHeight = 480;

    static void Initialize();

    static void PollEvents();

    static void SetKeyPressedCallback(GLFWkeyfun);

    static bool IsShouldBeClosed();

    static void SwapBuffers();

    static void Terminate();

    static void GetProcAddress(const char*);

private:
    static void GlfwErrorCallback(int what_is_this, const char* error);

    static GLFWwindow* m_windowInternal;
    static bool m_shouldBeClosed;
};
