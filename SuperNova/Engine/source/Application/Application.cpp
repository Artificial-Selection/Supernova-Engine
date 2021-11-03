#include <Engine/Application/Application.hpp>
#include <Engine/Application/IApplicationLayer.hpp>
#include <Engine/Application/Window.hpp>

#include <Engine/Input/Keyboard.hpp>
#include <Engine/Input/Mouse.hpp>


void ProcessInput()
{
    snv::Window::PollEvents();

    if (snv::Input::Keyboard::IsKeyPressed(snv::Input::KeyboardKey::Escape))
    {
        snv::Window::Close();
    }
}


namespace snv
{

Application::Application()
{
    Window::Init();
    Window::SetKeyCallback(Input::Keyboard::KeyCallback);
    Window::SetMouseButtonCallback(Input::Mouse::ButtonCallback);
    Window::SetMousePositionCallback(Input::Mouse::PositionCallback);
    Window::SetMouseWheelCallback(Input::Mouse::WheelCallback);
}

Application::~Application()
{
    for (auto layer : m_layers)
    {
        layer->OnDestroy();
        delete layer;
    }

    Window::Shutdown();
}


void Application::AddLayer(IApplicationLayer* layer)
{
    m_layers.push_back(layer);
    layer->OnCreate();
}

void Application::Run()
{
    while (Window::IsShouldBeClosed() == false)
    {
        ProcessInput();

        for (auto layer : m_layers)
        {
            layer->OnUpdate();
        }
    }
}

} // namespace snv
