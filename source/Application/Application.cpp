#include <Application/Application.hpp>

#include <Application/Window.hpp>
#include <Input/Keyboard.hpp>
#include <Input/Mouse.hpp>
#include <Renderer/Renderer.hpp>


const ui32 k_WindowWidth  = 1100;
const ui32 k_WindowHeight = 800;
// TODO(v.matushkin): Remove k_GraphicsApi duplication in Engine.cpp
const snv::GraphicsApi k_GraphicsApi = snv::GraphicsApi::DirectX12;


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
    Window::Init(k_WindowWidth, k_WindowHeight, "SuperNova-Engine", k_GraphicsApi);
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
