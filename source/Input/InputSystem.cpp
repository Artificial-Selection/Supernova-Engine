//
// Created by Devilast on 7/4/2021.
//

#include "InputSystem.hpp"
#include <Core/Log.hpp>
#include <Core/Window.hpp>
#include <Core/Utils/MemoryUtils.hpp>

namespace snv
{

    InputSystem::InputSystem()
    {
        MemZero(m_mappedKeys, sizeof(m_mappedKeys));
        for (int32_t i = '0'; i<='9'; i++)
        {
            m_mappedKeys[i] = static_cast<InputKey>(K_0 + i - '0');
        }
        
        for (int32_t i='A'; i<='Z'; i++)
        {
            m_mappedKeys[i] = static_cast<InputKey>(K_A + (i - 'A'));
        }

        for (int i = K_F1; i < K_F25; i++)
        {
            m_mappedKeys[i] = static_cast<InputKey>(K_F1 + (i - K_F1));
        }

        Window::SetKeyPressedCallback(KeyCallback);
    }

    void InputSystem::KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
    {
        switch (action)
        {
            case InputAction::PRESS :
            {
                LOG_INFO("KEY WAS PRESSED");
                break;
            }
            case InputAction::RELEASED :
            {
                LOG_INFO("KEY WAS RELEASED");
                break;
            }
            default:
            {
                break;
            }
        }
    }


    void InputSystem::Update(float elapsedTime)
    {
        Window::PollEvents();
    }
}


