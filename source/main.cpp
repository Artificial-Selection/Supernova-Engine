#include <Core/Log.hpp>
#include <Core/Window.hpp>

#include <Renderer/Renderer.hpp>
#include <Renderer/OpenGL/GLShader.hpp>
#include <Renderer/OpenGL/GLTexture.hpp>

#include <Entity/GameObject.hpp>
#include <Components/Transform.hpp>
#include <Components/Camera.hpp>

#include <Assets/AssetDatabase.hpp>
#include <Assets/Model.hpp>

#include <Input/Keyboard.hpp>
#include <Input/Mouse.hpp>

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

void TestSecondRequest(i32 value)
{
    LOG_INFO("TestSecondRequest");
}

void ProcessInput(const snv::Window& window, snv::Transform& cameraTransform, snv::Transform& modelTransform)
{
    window.PollEvents();

    if (snv::Input::Keyboard::IsKeyPressed(snv::Input::KeyboardKey::Escape))
    {
        window.Close();
    }

    constexpr f32 step = 0.05f;

    if (snv::Input::Keyboard::IsKeyPressed(snv::Input::KeyboardKey::W))
    {
        cameraTransform.Translate(0.0f, 0.0f, step);
    }
    if (snv::Input::Keyboard::IsKeyPressed(snv::Input::KeyboardKey::S))
    {
        cameraTransform.Translate(0.0f, 0.0f, -step);
    }
    if (snv::Input::Keyboard::IsKeyPressed(snv::Input::KeyboardKey::A))
    {
        cameraTransform.Translate(step, 0.0f, 0.0f);
    }
    if (snv::Input::Keyboard::IsKeyPressed(snv::Input::KeyboardKey::D))
    {
        cameraTransform.Translate(-step, 0.0f, 0.0f);
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

void Render(const snv::Window& window, const snv::ModelPtr model, const snv::GLShader& shader)
{
    // NOTE(v.matushkin): Don't need to clear stencil rn, just to test that is working
    snv::Renderer::Clear(static_cast<snv::BufferBit>(snv::BufferBit::Color | snv::BufferBit::Depth | snv::BufferBit::Stencil));

    for (const auto&[mesh, material] : model->GetMeshes())
    {
        const auto textureHandle = material.GetTextureHandle();
        shader.SetInt1("_DiffuseTexture", 0); // Can set only once?
        snv::Renderer::DrawGraphicsBuffer(mesh.GetHandle(), textureHandle, mesh.GetIndexCount(), mesh.GetVertexCount());
    }

    window.SwapBuffers();
}

void Update(f32 deltaTime)
{
    //TODO Implement
}


int main()
{
    //Log::Init( spdlog::level::trace );
    LOG_TRACE("SuperNova-Engine Init");

    snv::Window window(k_WindowWidth, k_WindowHeight, "SuperNova-Engine");
    window.SetKeyCallback(snv::Input::Keyboard::KeyCallback);
    window.SetMouseButtonCallback(snv::Input::Mouse::ButtonCallback);

    snv::Renderer::Init();
    snv::Renderer::SetViewport(0, 0, k_WindowWidth, k_WindowHeight);
    snv::Renderer::SetClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    snv::Renderer::EnableDepthTest();
    snv::Renderer::SetDepthFunction(snv::DepthFunction::Less);

    const auto vertexSource = LoadShaderFromFile(k_VertexSourcePath);
    const auto fragmentSource = LoadShaderFromFile(k_FragmentSourcePath);
    snv::GLShader modelShader(vertexSource.get(), fragmentSource.get());
    modelShader.Bind();

    snv::GLTexture texture;

    const auto sponzaModel = snv::AssetDatabase::LoadAsset<snv::Model>(k_SponzaObjPath);

    snv::GameObject modelGameObject;
    auto& modelTransform = modelGameObject.GetComponent<snv::Transform>();
    modelTransform.SetScale(0.005f);

    snv::GameObject cameraGameObject;
    const auto& camera = modelGameObject.AddComponent<snv::Camera>(90.0f, f32(k_WindowWidth) / k_WindowHeight, 0.1f, 100.0f);
    const auto& projectionMatrix = camera.GetProjectionMatrix();
    auto& cameraTransform = cameraGameObject.GetComponent<snv::Transform>();

    const i32 maxFPS = 60;
    const auto maxPeriod = 1.0 / maxFPS;

    auto startTime = std::chrono::high_resolution_clock::now().time_since_epoch();

    while (window.IsShouldBeClosed() == false)
    {
        //TODO FPS lock
        auto currentTime = std::chrono::high_resolution_clock::now().time_since_epoch();
        auto elapsed = 1.0 / (currentTime - startTime).count();
        //LOG_INFO("current lag is {}", elapsed);
        startTime = currentTime;

        ProcessInput(window, cameraTransform, modelTransform);
        Update(elapsed);

        modelShader.SetMatrix4("_ObjectToWorld", modelTransform.GetMatrix());
        modelShader.SetMatrix4("_MatrixP", projectionMatrix);
        modelShader.SetMatrix4("_MatrixV", cameraTransform.GetMatrix());

        Render(window, sponzaModel, modelShader);
    }

    LOG_TRACE("SuperNova-Engine Shutdown");

    snv::Renderer::Shutdown();

    return 0;
}
