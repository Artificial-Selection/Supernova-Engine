#include <Core/Log.hpp>
#include <Core/Window.hpp>

#include <Renderer/Renderer.hpp>
#include <Renderer/OpenGL/GLShader.hpp>

#include <Entity/GameObject.hpp>
#include <Components/Transform.hpp>
#include <Assets/Model.hpp>

#include <Input/Keyboard.hpp>
#include <Input/Mouse.hpp>

#include <glm/gtc/matrix_transform.hpp>

#include <chrono>
#include <memory>
#include <string_view>
#include <fstream>
#include <filesystem>


constexpr ui32 k_WindowWidth  = 1100;
constexpr ui32 k_WindowHeight = 800;

const char* k_SponzaObjPath   = "../../assets/models/Sponza/sponza.obj";

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

void ProcessInput(const snv::Window& window, glm::mat4& transform, snv::Transform& model)
{
    window.PollEvents();

    if (snv::Input::Keyboard::IsKeyPressed(snv::Input::KeyboardKey::Escape))
    {
        window.Close();
    }

    constexpr f32 step = 0.05f;

    if (snv::Input::Keyboard::IsKeyPressed(snv::Input::KeyboardKey::W))
    {
        transform = glm::translate(transform, glm::vec3(0.0f, 0.0f, step));
    }
    if (snv::Input::Keyboard::IsKeyPressed(snv::Input::KeyboardKey::S))
    {
        transform = glm::translate(transform, glm::vec3(0.0f, 0.0f, -step));
    }
    if (snv::Input::Keyboard::IsKeyPressed(snv::Input::KeyboardKey::A))
    {
        transform = glm::translate(transform, glm::vec3(step, 0.0f, 0.0f));
    }
    if (snv::Input::Keyboard::IsKeyPressed(snv::Input::KeyboardKey::D))
    {
        transform = glm::translate(transform, glm::vec3(-step, 0.0f, 0.0f));
    }

    if (snv::Input::Keyboard::IsKeyPressed(snv::Input::KeyboardKey::Z))
    {
        transform = glm::translate(transform, glm::vec3(0.0f, step, 0.0f));
    }
    if (snv::Input::Keyboard::IsKeyPressed(snv::Input::KeyboardKey::X))
    {
        transform = glm::translate(transform, glm::vec3(0.0f, -step, 0.0f));
    }

    if (snv::Input::Keyboard::IsKeyPressed(snv::Input::KeyboardKey::Q)
        || snv::Input::Mouse::IsButtonPressed(snv::Input::MouseButton::Left))
    {
        model.Rotate(0.0f, -1.0f, 0.0f);
    }
    if (snv::Input::Keyboard::IsKeyPressed(snv::Input::KeyboardKey::E)
        || snv::Input::Mouse::IsButtonPressed(snv::Input::MouseButton::Right))
    {
        model.Rotate(0.0f, 1.0f, 0.0f);
    }
}

void Render(const snv::Window& window, const snv::Model& model)
{
    // NOTE(v.matushkin): Don't need to clear stencil rn, just to test that is working
    snv::Renderer::Clear(static_cast<snv::BufferBit>(snv::BufferBit::Color | snv::BufferBit::Depth | snv::BufferBit::Stencil));

    for (const auto& mesh : model.GetMeshes())
    {
        snv::Renderer::DrawGraphicsBuffer(mesh.GetHandle(), mesh.GetIndexCount(), mesh.GetVertexCount());
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
    snv::GLShader triangleShader(vertexSource.get(), fragmentSource.get());
    triangleShader.Bind();

    const auto model = snv::Model::LoadAsset(k_SponzaObjPath);

    snv::GameObject gameObject;
    auto& transform = gameObject.GetComponent<snv::Transform>();
    transform.SetScale(0.005f);

    auto view = glm::identity<glm::mat4>();
    auto projection = glm::perspective(glm::radians(90.0f), f32(k_WindowWidth) / k_WindowHeight, 0.1f, 100.0f);

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

        ProcessInput(window, view, transform);
        Update(elapsed);

        triangleShader.SetMatrix4("_ObjectToWorld", transform.GetMatrix());
        triangleShader.SetMatrix4("_MatrixP", projection);
        triangleShader.SetMatrix4("_MatrixV", view);

        Render(window, model);
    }

    LOG_TRACE("SuperNova-Engine Shutdown");

    snv::Renderer::Shutdown();

    return 0;
}
