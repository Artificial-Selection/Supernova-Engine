#include <Core/Log.hpp>
#include <Core/Window.hpp>

#include <Renderer/Renderer.hpp>
#include <Renderer/OpenGL/GLShader.hpp>
#include <Renderer/OpenGL/GLTexture.hpp>

#include <Entity/GameObject.hpp>
#include <Components/Camera.hpp>
#include <Components/CameraController.hpp>
#include <Components/Transform.hpp>

#include <Assets/AssetDatabase.hpp>
#include <Assets/Model.hpp>
#include <Assets/Texture.hpp>

#include <Input/Keyboard.hpp>
#include <Input/Mouse.hpp>

#include <Utils/Time.hpp>

#include <chrono>
#include <memory>
#include <string_view>
#include <fstream>
#include <filesystem>


constexpr ui32 k_WindowWidth  = 1100;
constexpr ui32 k_WindowHeight = 800;

const char* k_SponzaObjPath = "../../assets/models/Sponza/sponza.obj";

const char* k_VertexSourcePath   = "../../assets/shaders/triangle_vs.glsl";
const char* k_FragmentSourcePath = "../../assets/shaders/triangle_fs.glsl";


std::unique_ptr<char[]> LoadShaderFromFile(std::string_view shaderPath)
{
    const auto size = std::filesystem::file_size(shaderPath);
    auto shaderSource = std::make_unique<char[]>(size + 1);

    std::ifstream shaderFile(shaderPath, std::ios::binary | std::ios::in);
    shaderFile.read(shaderSource.get(), size);

    return shaderSource;
}


void ProcessInput(const snv::Window& window, snv::Transform& cameraTransform, snv::Transform& modelTransform)
{
    snv::Window::PollEvents();

    if (snv::Input::Keyboard::IsKeyPressed(snv::Input::KeyboardKey::Escape))
    {
        snv::Window::Close();
    }

    if (snv::Input::Keyboard::IsKeyPressed(snv::Input::KeyboardKey::Z))
    {
        cameraTransform.Translate(0.0f, step, 0.0f);
    }
    if (snv::Input::Keyboard::IsKeyPressed(snv::Input::KeyboardKey::X))
    {
        cameraTransform.Translate(0.0f, -step, 0.0f);
    }

    if (snv::Input::Keyboard::IsKeyPressed(snv::Input::KeyboardKey::Q)
        || snv::Input::Mouse::IsButtonPressed(snv::Input::MouseButton::Left))
    {
        modelTransform.Rotate(0.0f, -1.0f, 0.0f);
    }
    if (snv::Input::Keyboard::IsKeyPressed(snv::Input::KeyboardKey::E)
        || snv::Input::Mouse::IsButtonPressed(snv::Input::MouseButton::Right))
    {
        modelTransform.Rotate(0.0f, 1.0f, 0.0f);
    }
}

void Render(const snv::ModelPtr model, const snv::GLShader& shader)
{
    // NOTE(v.matushkin): Don't need to clear stencil rn, just to test that is working
    snv::Renderer::Clear(static_cast<snv::BufferBit>(snv::BufferBit::Color | snv::BufferBit::Depth | snv::BufferBit::Stencil));

    for (const auto& [mesh, material] : model->GetMeshes())
    {
        const auto textureHandle = material.GetBaseColorMap()->GetTextureHandle();
        shader.SetInt1("_DiffuseTexture", 0); // Can set only once?
        snv::Renderer::DrawGraphicsBuffer(mesh.GetHandle(), textureHandle, mesh.GetIndexCount(), mesh.GetVertexCount());
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

    snv::Renderer::Init();
    snv::Renderer::SetViewport(0, 0, k_WindowWidth, k_WindowHeight);
    snv::Renderer::SetClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    snv::Renderer::EnableDepthTest();
    snv::Renderer::SetDepthFunction(snv::DepthFunction::Less);

    const auto vertexSource = LoadShaderFromFile(k_VertexSourcePath);
    const auto fragmentSource = LoadShaderFromFile(k_FragmentSourcePath);
    snv::GLShader modelShader(vertexSource.get(), fragmentSource.get());
    modelShader.Bind();

    const auto sponzaLoadStart = std::chrono::high_resolution_clock::now();
    const auto sponzaModel = snv::AssetDatabase::LoadAsset<snv::Model>(k_SponzaObjPath);
    const auto sponzaLoadTime = std::chrono::high_resolution_clock::now() - sponzaLoadStart;
    LOG_INFO("Sponza loading time: {}ms", std::chrono::duration_cast<std::chrono::milliseconds>(sponzaLoadTime).count());

    snv::GameObject sponzaGameObject;
    auto& sponzaTransform = sponzaGameObject.GetComponent<snv::Transform>();
    sponzaTransform.SetScale(0.005f);

    snv::GameObject cameraGameObject;
    const auto& camera = cameraGameObject.AddComponent<snv::Camera>(90.0f, f32(k_WindowWidth) / k_WindowHeight, 0.1f, 100.0f);
    const auto& projectionMatrix = camera.GetProjectionMatrix();
    auto& cameraTransform = cameraGameObject.GetComponent<snv::Transform>();

    while (snv::Window::IsShouldBeClosed() == false)
    {
        snv::Time::Update();
        Update(elapsed);

        modelShader.SetMatrix4("_ObjectToWorld", sponzaTransform.GetMatrix());
        modelShader.SetMatrix4("_MatrixP", projectionMatrix);
        modelShader.SetMatrix4("_MatrixV", cameraTransform.GetMatrix());

        Render(sponzaModel, modelShader);
    }

    LOG_TRACE("SuperNova-Engine Shutdown");

    snv::Renderer::Shutdown();
    snv::Window::Shutdown();

    return 0;
}
