#include <Engine/Engine.hpp>

#include <Engine/Application/Window.hpp>
#include <Engine/Assets/AssetDatabase.hpp>
#include <Engine/Assets/Shader.hpp>
#include <Engine/Components/Camera.hpp>
#include <Engine/Components/CameraController.hpp>
#include <Engine/Components/Transform.hpp>
#include <Engine/Core/Log.hpp>
#include <Engine/Renderer/Renderer.hpp>
#include <Engine/Utils/Time.hpp>

#include <chrono>


const f32 k_MovementSpeed = 2.0f;
const f32 k_MovementBoost = 5.0f;

const char* k_SponzaObjPath = "../../assets/models/Sponza/sponza.obj";
const char* k_ShaderPath    = "../../assets/shaders/triangle";

const snv::GraphicsApi k_GraphicsApi = snv::GraphicsApi::DirectX12;


namespace snv
{

void Engine::OnCreate()
{
    // Log::Init( spdlog::level::trace );
    LOG_TRACE("SuperNova-Engine Init");
    Time::Init();

    const auto windowWidth  = snv::Window::GetWidth();
    const auto windowHeight = snv::Window::GetHeight();

    Renderer::Init(k_GraphicsApi);
    Renderer::SetViewport(0, 0, windowWidth, windowHeight);
    Renderer::SetClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    Renderer::EnableDepthTest();
    Renderer::SetDepthFunction(DepthFunction::Less);


    (void) AssetDatabase::LoadAsset<snv::Shader>(k_ShaderPath);

    const auto sponzaLoadStart = std::chrono::high_resolution_clock::now();
    m_sponzaModel              = AssetDatabase::LoadAsset<Model>(k_SponzaObjPath);
    const auto sponzaLoadTime  = std::chrono::high_resolution_clock::now() - sponzaLoadStart;
    LOG_INFO("Sponza loading time: {}ms", std::chrono::duration_cast<std::chrono::milliseconds>(sponzaLoadTime).count());

    m_sponzaGO.GetComponent<snv::Transform>().SetScale(0.005f);

    m_camera.AddComponent<Camera>(90.0f, f32(windowWidth) / windowHeight, 0.1f, 100.0f);
    auto& cameraController = m_camera.AddComponent<CameraController>(k_MovementSpeed, k_MovementBoost);
}

void Engine::OnDestroy()
{
    LOG_TRACE("SuperNova-Engine Shutdown");

    Renderer::Shutdown();
}

void Engine::OnUpdate()
{
    Time::Update();

    auto& cameraController = m_camera.GetComponent<CameraController>();
    cameraController.OnUpdate();

    auto& sponzaTransform = m_sponzaGO.GetComponent<snv::Transform>();
    Renderer::RenderFrame(sponzaTransform.GetMatrix());
}

} // namespace snv
