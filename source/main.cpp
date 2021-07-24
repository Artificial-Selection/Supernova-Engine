#include <Core/Log.hpp>
#include <Core/Window.hpp>

#include <Renderer/Renderer.hpp>

#include <Entity/GameObject.hpp>
#include <Components/Camera.hpp>
#include <Components/CameraController.hpp>
#include <Components/MeshRenderer.hpp>
#include <Components/Transform.hpp>

#include <Assets/AssetDatabase.hpp>
#include <Assets/Model.hpp>
#include <Assets/Shader.hpp>

#include <Input/Keyboard.hpp>
#include <Input/Mouse.hpp>

#include <Utils/Time.hpp>

#include <chrono>


constexpr ui32 k_WindowWidth  = 1100;
constexpr ui32 k_WindowHeight = 800;

constexpr f32 k_MovementSpeed = 2.0f;
constexpr f32 k_MovementBoost = 5.0f;

const char* k_SponzaObjPath = "../../assets/models/Sponza/sponza.obj";
const char* k_ShaderPath    = "../../assets/shaders/triangle";


void ProcessInput()
{
    snv::Window::PollEvents();

    if (snv::Input::Keyboard::IsKeyPressed(snv::Input::KeyboardKey::Escape))
    {
        snv::Window::Close();
    }
}

void Update(snv::CameraController& cameraController)
{
    cameraController.OnUpdate();
}

void Render(const glm::mat4x4& modelM)
{
    // NOTE(v.matushkin): Don't need to clear stencil rn, just to test that is working
    snv::Renderer::Clear(static_cast<snv::BufferBit>(snv::BufferBit::Color | snv::BufferBit::Depth | snv::BufferBit::Stencil));

    snv::Renderer::RenderFrame(modelM);

    snv::Window::SwapBuffers();
}


int main()
{
    //Log::Init( spdlog::level::trace );
    LOG_TRACE("SuperNova-Engine Init");
    snv::Time::Init();

    snv::Window::Init(k_WindowWidth, k_WindowHeight, "SuperNova-Engine");
    snv::Window::SetKeyCallback(snv::Input::Keyboard::KeyCallback);
    snv::Window::SetMouseButtonCallback(snv::Input::Mouse::ButtonCallback);
    snv::Window::SetMousePositionCallback(snv::Input::Mouse::PositionCallback);
    snv::Window::SetMouseWheelCallback(snv::Input::Mouse::WheelCallback);

    snv::Renderer::Init();
    snv::Renderer::SetViewport(0, 0, k_WindowWidth, k_WindowHeight);
    snv::Renderer::SetClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    snv::Renderer::EnableDepthTest();
    snv::Renderer::SetDepthFunction(snv::DepthFunction::Less);

    (void)snv::AssetDatabase::LoadAsset<snv::Shader>(k_ShaderPath);

    const auto sponzaLoadStart = std::chrono::high_resolution_clock::now();
    const auto sponzaModel = snv::AssetDatabase::LoadAsset<snv::Model>(k_SponzaObjPath);
    const auto sponzaLoadTime = std::chrono::high_resolution_clock::now() - sponzaLoadStart;
    LOG_INFO("Sponza loading time: {}ms", std::chrono::duration_cast<std::chrono::milliseconds>(sponzaLoadTime).count());

    snv::GameObject sponzaGameObject;
    auto& sponzaTransform = sponzaGameObject.GetComponent<snv::Transform>();
    sponzaTransform.SetScale(0.005f);

    snv::GameObject cameraGameObject;
    cameraGameObject.AddComponent<snv::Camera>(90.0f, f32(k_WindowWidth) / k_WindowHeight, 0.1f, 100.0f);
    auto& cameraController = cameraGameObject.AddComponent<snv::CameraController>(k_MovementSpeed, k_MovementBoost);

    while (snv::Window::IsShouldBeClosed() == false)
    {
        snv::Time::Update();
        ProcessInput();
        Update(cameraController);

        Render(sponzaTransform.GetMatrix());
    }

    LOG_TRACE("SuperNova-Engine Shutdown");

    snv::Renderer::Shutdown();
    snv::Window::Shutdown();

    return 0;
}
