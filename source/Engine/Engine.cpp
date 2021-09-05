#include <Engine/Engine.hpp>

#include <Core/Log.hpp>
#include <Core/Window.hpp>

#include <Assets/AssetDatabase.hpp>
#include <Assets/Shader.hpp>

#include <Components/Camera.hpp>
#include <Components/CameraController.hpp>
#include <Components/MeshRenderer.hpp>
#include <Components/Transform.hpp>

#include <Renderer/Renderer.hpp>

#include <Utils/Time.hpp>

#include <chrono>


const ui32 k_WindowWidth  = 1100;
const ui32 k_WindowHeight = 800;

constexpr f32 k_MovementSpeed = 2.0f;
constexpr f32 k_MovementBoost = 5.0f;

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

    Renderer::Init(k_GraphicsApi);
    Renderer::SetViewport(0, 0, k_WindowWidth, k_WindowHeight);
    Renderer::SetClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    Renderer::EnableDepthTest();
    Renderer::SetDepthFunction(DepthFunction::Less);


    (void) AssetDatabase::LoadAsset<snv::Shader>(k_ShaderPath);

    const auto sponzaLoadStart = std::chrono::high_resolution_clock::now();
    m_sponzaModel              = AssetDatabase::LoadAsset<Model>(k_SponzaObjPath);
    const auto sponzaLoadTime  = std::chrono::high_resolution_clock::now() - sponzaLoadStart;
    LOG_INFO("Sponza loading time: {}ms", std::chrono::duration_cast<std::chrono::milliseconds>(sponzaLoadTime).count());

    m_sponzaGO.GetComponent<snv::Transform>().SetScale(0.005f);

    m_camera.AddComponent<Camera>(90.0f, f32(k_WindowWidth) / k_WindowHeight, 0.1f, 100.0f);
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
