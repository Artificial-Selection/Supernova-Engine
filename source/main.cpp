#include <Core/Log.hpp>
#include <Renderer/Renderer.hpp>
#include <Renderer/OpenGL/GLShader.hpp>
#include <Entity/GameObject.hpp>
#include <Components/Transform.hpp>
#include <Assets/Model.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>
#include <memory>
#include <string_view>
#include <fstream>
#include <filesystem>


constexpr ui32 k_ScreenWidth = 1100;
constexpr ui32 k_ScreenHeight = 800;

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

void GlfwErrorCallback(i32 what_is_this, const char* error)
{
    LOG_ERROR("GLFW: {}", error);
}

void TestSecondRequest(i32 value)
{
    LOG_INFO("TestSecondRequest");
}

void ProcessInput(GLFWwindow* window, glm::mat4& transform, snv::Transform& model)
{
    glfwPollEvents();

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }

    constexpr f32 step = 0.05f;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        transform = glm::translate(transform, glm::vec3(0.0f, 0.0f, step));
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        transform = glm::translate(transform, glm::vec3(0.0f, 0.0f, -step));
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
        transform = glm::translate(transform, glm::vec3(step, 0.0f, 0.0f));
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
        transform = glm::translate(transform, glm::vec3(-step, 0.0f, 0.0f));
    }

    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
    {
        transform = glm::translate(transform, glm::vec3(0.0f, step, 0.0f));
    }
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
    {
        transform = glm::translate(transform, glm::vec3(0.0f, -step, 0.0f));
    }

    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
    {
        model.Rotate(0.0f, -1.0f, 0.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
    {
        model.Rotate(0.0f, 1.0f, 0.0f);
    }
}

void Render(GLFWwindow* window, const snv::Model& model)
{
    // NOTE(v.matushkin): Don't need to clear stencil rn, just to test that is working
    snv::Renderer::Clear(static_cast<snv::BufferBit>(snv::BufferBit::Color | snv::BufferBit::Depth | snv::BufferBit::Stencil));

    for (const auto& mesh : model.GetMeshes())
    {
        snv::Renderer::DrawGraphicsBuffer(mesh.GetHandle(), mesh.GetIndexCount(), mesh.GetVertexCount());
    }

    glfwSwapBuffers(window);
}

void Update(f32 deltaTime)
{
    //TODO Implement
}

//struct Position
//{
//    float X;
//    float Y;
//};
//
//struct Rotation
//{
//    float Euler;
//};

//void UpdateView()
//{
//    auto currentView = ComponentPool::Instance().GetViewByComponents<Position, Rotation>();
//    for(auto [entity, pos, rot]: currentView.each()) {
//        LOG_INFO("Current Entity {}", entity);
//        LOG_INFO("Current Position {}", pos.X);
//        LOG_INFO("Current Rotation {}", rot.Euler);
//    }
//}
//
//void Test()
//{
//    for (auto i = 0u; i < 10u; ++i)
//    {
//        const auto entity = ComponentPool::Instance().Create();
//        ComponentPool::Instance().Emplace<Position>(entity, static_cast<float>(i * 1.f, i * 1.f));
//        if (i % 2 == 0)
//        {
//            ComponentPool::Instance().Emplace<Rotation>(entity, static_cast<float>(i * .1f));
//        }
//    }
//    UpdateView();
//}


int main()
{
    //Log::Init( spdlog::level::trace );
    LOG_TRACE("SuperNova-Engine Init");

    glfwSetErrorCallback(GlfwErrorCallback);
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, false);
#ifdef SNV_ENABLE_DEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif

    GLFWwindow* window = glfwCreateWindow(k_ScreenWidth, k_ScreenHeight, "SuperNova-Engine", nullptr, nullptr);
    if (window == nullptr)
    {
        LOG_CRITICAL("Failed to create GLFW window");
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (gladLoadGLLoader((GLADloadproc) glfwGetProcAddress) == 0)
    {
        LOG_CRITICAL("Failed to initialize GLAD");
        return -1;
    }

    snv::Renderer::Init();
    snv::Renderer::SetViewport(0, 0, k_ScreenWidth, k_ScreenHeight);
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
    auto projection = glm::perspective(glm::radians(90.0f), f32(k_ScreenWidth) / k_ScreenHeight, 0.1f, 100.0f);

    const i32 maxFPS = 60;
    const auto maxPeriod = 1.0 / maxFPS;

    auto startTime = std::chrono::high_resolution_clock::now().time_since_epoch();

    while (glfwWindowShouldClose(window) == 0)
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
    glfwTerminate();

    return 0;
}
