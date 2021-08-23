#include "Application.hpp"
#include "Window.hpp"

namespace SuperNova
{
Application::Application(std::string name)
{
}

    void Application::Init()
    {
        m_window = std::make_unique<snv::Window>();
    }

    void Application::Run()
    {

    }
};