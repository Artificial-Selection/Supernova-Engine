//
// Created by Devilast on 7/4/2021.
//
#pragma once

#include <Input/InputUtils.hpp>

class GLFWwindow;

namespace snv
{
    class InputSystem
    {
    public:
        static void Update(float elapsedTime);

        InputSystem();

    private:
        static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

        InputKey m_mappedKeys[348]{};
    };
}

