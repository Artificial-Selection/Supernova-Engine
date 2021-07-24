#include <Core/Log.hpp>
#include <Core/Window.hpp>

#include <Renderer/Renderer.hpp>
#include <Renderer/OpenGL/GLShader.hpp>
#include <Renderer/OpenGL/GLTexture.hpp>

#include <Entity/GameObject.hpp>
#include <Components/Camera.hpp>
#include <Components/CameraController.hpp>
#include <Components/MeshRenderer.hpp>
#include <Components/Transform.hpp>

#include <Assets/AssetDatabase.hpp>
#include <Assets/Material.hpp>
#include <Assets/Mesh.hpp>
#include <Assets/Model.hpp>
#include <Assets/Texture.hpp>
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

void Render(const snv::ModelPtr model, const glm::mat4x4& modelM, const glm::mat4x4& viewM, const glm::mat4x4& projectionM)
{
    const auto& gameObjects = model->GetGameObjects();
    const auto shaderHandle = gameObjects[0].GetComponent<snv::MeshRenderer>().GetMaterial()->GetShader()->GetHandle();

    // NOTE(v.matushkin): Don't need to clear stencil rn, just to test that is working
    snv::Renderer::Clear(static_cast<snv::BufferBit>(snv::BufferBit::Color | snv::BufferBit::Depth | snv::BufferBit::Stencil));

    snv::Renderer::StartFrame(shaderHandle, modelM, viewM, projectionM);

    for (const auto& gameObject : gameObjects)
    {
        const auto& meshRenderer = gameObject.GetComponent<snv::MeshRenderer>();

        const auto material = meshRenderer.GetMaterial();
        const auto textureHandle = material->GetBaseColorMap()->GetTextureHandle();

        const auto mesh = meshRenderer.GetMesh();
        const auto meshHandle = mesh->GetHandle();
        const auto indexCount = mesh->GetIndexCount();
        const auto vertexCount = mesh->GetVertexCount();

        snv::Renderer::DrawGraphicsBuffer(textureHandle, meshHandle, indexCount, vertexCount);
    }

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
    const auto& camera = cameraGameObject.AddComponent<snv::Camera>(90.0f, f32(k_WindowWidth) / k_WindowHeight, 0.1f, 100.0f);
    auto& cameraController = cameraGameObject.AddComponent<snv::CameraController>(k_MovementSpeed, k_MovementBoost);
    const auto& projectionMatrix = camera.GetProjectionMatrix();
    auto& cameraTransform = cameraGameObject.GetComponent<snv::Transform>();

    while (snv::Window::IsShouldBeClosed() == false)
    {
        snv::Time::Update();
        ProcessInput();
        Update(cameraController);

        Render(sponzaModel, sponzaTransform.GetMatrix(), cameraTransform.GetMatrix(), projectionMatrix);
    }

    LOG_TRACE("SuperNova-Engine Shutdown");

    snv::Renderer::Shutdown();
    snv::Window::Shutdown();

    return 0;
}
