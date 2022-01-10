#include <Engine/Engine.hpp>
#include <Engine/EngineSettings.hpp>

#include <Engine/Assets/AssetDatabase.hpp>
#include <Engine/Assets/Shader.hpp>
#include <Engine/Components/Camera.hpp>
#include <Engine/Components/CameraController.hpp>
#include <Engine/Components/Transform.hpp>
#include <Engine/Core/Log.hpp>
#include <Engine/Renderer/Renderer.hpp>
#include <Engine/Utils/Time.hpp>

#include <chrono>


static const f32 k_MovementSpeed = 2.0f;
static const f32 k_MovementBoost = 5.0f;

static const char* k_AssetDir      = "../../assets/";
static const char* k_SponzaObjPath = "Sponza/sponza.obj";
static const char* k_ShaderName    = "Main";


namespace snv
{

void Engine::OnCreate()
{
    // Log::Init( spdlog::level::trace );
    LOG_TRACE("SuperNova-Engine Init");
    Time::Init();

    const auto renderWidth  = EngineSettings::GraphicsSettings.RenderWidth;
    const auto renderHeight = EngineSettings::GraphicsSettings.RenderHeight;

    AssetDatabase::Init(k_AssetDir);

    Renderer::Init();
    Renderer::SetViewport(0, 0, renderWidth, renderHeight);
    Renderer::SetClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    Renderer::EnableDepthTest();
    Renderer::SetDepthFunction(CompareFunction::Less);

    (void) AssetDatabase::LoadAsset<Shader>(k_ShaderName);

    const auto sponzaLoadStart = std::chrono::high_resolution_clock::now();
    m_sponzaModel              = AssetDatabase::LoadAsset<Model>(k_SponzaObjPath);
    const auto sponzaLoadTime  = std::chrono::high_resolution_clock::now() - sponzaLoadStart;
    LOG_INFO("Sponza loading time: {}ms", std::chrono::duration_cast<std::chrono::milliseconds>(sponzaLoadTime).count());

    m_sponzaGO.GetComponent<Transform>().SetScale(0.005f);

    m_camera.AddComponent<Camera>(90.0f, f32(renderWidth) / renderHeight, 0.1f, 100.0f);
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

    auto& sponzaTransform = m_sponzaGO.GetComponent<Transform>();
    Renderer::RenderFrame(sponzaTransform.GetMatrix());
}

} // namespace snv
