//
// Created by Devilast on 08.09.2020.
//
#include <Core/Log.hpp>
#include <Library/ObservableField.h>
#include <Renderer/OpenGL/GLBackend.hpp>
#include <Renderer/OpenGL/GLShader.hpp>
#include <Core/ComponentFactory.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <chrono>
#include <memory>
#include <string_view>
#include <fstream>
#include <filesystem>


constexpr ui32 k_ScreenWidth = 800;
constexpr ui32 k_ScreenHeight = 600;

const char* k_VertexSourcePath = "../../assets/shaders/triangle_vs.glsl";
const char* k_FragmentSourcePath = "../../assets/shaders/triangle_fs.glsl";

using ComponentPool = ComponentFactory<entt::entity>;

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

void ProcessInput(GLFWwindow* window)
{
    glfwPollEvents();

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }
}

void Render(GLFWwindow* window)
{
    // GG WP worth it
    auto bufferBitMask = static_cast<GLBufferBit>((ui32) GLBufferBit::Color | (ui32) GLBufferBit::Depth);
    GLBackend::Clear(bufferBitMask);

    GLBackend::DrawArrays(3);

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
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_CORE_PROFILE, GLFW_OPENGL_CORE_PROFILE);
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

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
    {
        LOG_CRITICAL("Failed to initialize GLAD");
        return -1;
    }

    GLBackend::Init();
    GLBackend::SetViewport(0, 0, k_ScreenWidth, k_ScreenHeight);
    GLBackend::SetClearColor(0.1f, 0.1f, 0.80f, 1.0f);
    GLBackend::EnableDepthTest();
    GLBackend::SetDepthFunction(GLDepthFunction::Less);

    const auto vertexSource = LoadShaderFromFile(k_VertexSourcePath);
    const auto fragmentSource = LoadShaderFromFile(k_FragmentSourcePath);
    GLShader triangleShader(vertexSource.get(), fragmentSource.get());
    triangleShader.Bind();


    // Triangle buffer
    f32 vertices[] = {
            // positions        // colors
            0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f,
            -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f
    };
    ui32 VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(f32), (void*) 0);
    glEnableVertexAttribArray(0);
    // color
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(f32), (void*) (3 * sizeof(f32)));
    glEnableVertexAttribArray(1);


    const i32 maxFPS = 60;
    const auto maxPeriod = 1.0 / maxFPS;
    // NOTE: glfwGetTime() ?
    auto startTime = std::chrono::high_resolution_clock::now().time_since_epoch();

    while (!glfwWindowShouldClose(window))
    {
        //TODO FPS lock
        auto currentTime = std::chrono::high_resolution_clock::now().time_since_epoch();
        auto elapsed = 1.0 / (currentTime - startTime).count();
        //LOG_INFO("current lag is {}", elapsed);
        startTime = currentTime;

        ProcessInput(window);
        Update(elapsed);
        Render(window);
    }

    LOG_TRACE("SuperNova-Engine Shutdown");

    glfwTerminate();

    return 0;
}
