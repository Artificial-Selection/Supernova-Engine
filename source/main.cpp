//
// Created by Devilast on 08.09.2020.
//
#include <Core/Log.hpp>
#include <Renderer/OpenGL/GLBackend.hpp>
#include <Renderer/OpenGL/GLShader.hpp>
#include <Entity/GameObject.hpp>
#include <Components/Transform.hpp>
#include <Core/Window.hpp>

#include <glad/glad.h>
#include <glm/gtx/string_cast.hpp>
#include <Input/InputSystem.hpp>

#include <chrono>
#include <memory>
#include <string_view>
#include <fstream>
#include <filesystem>


constexpr ui32 k_ScreenWidth = 800;
constexpr ui32 k_ScreenHeight = 600;

const char* k_VertexSourcePath = "../../assets/shaders/triangle_vs.glsl";
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


//void ProcessInput(GLFWwindow* window, svn::InputSystemEditor* inputSystemBase)
//{
//    glfwPollEvents();
//
//    glfwSetKeyCallback(window, svn::InputSystemEditor::MessageTest);
//    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
//    {
//        glfwSetWindowShouldClose(window, true);
//    }
//}

void Render()
{
    // GG WP worth it
    auto bufferBitMask = static_cast<snv::GLBufferBit>((ui32) snv::GLBufferBit::Color | (ui32) snv::GLBufferBit::Depth);
    snv::GLBackend::Clear(bufferBitMask);

    snv::GLBackend::DrawArrays(3);

    Window::SwapBuffers();
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
    LOG_TRACE("SuperNova-Engine Init");

    Window::Initialize();
    std::unique_ptr<snv::InputSystem> _inputSystem = std::make_unique<snv::InputSystem>();
    //snv::InputSystemEditor::Initialize();

    if (gladLoadGLLoader((GLADloadproc) Window::GetProcAddress) == 0)
    {
        LOG_CRITICAL("Failed to initialize GLAD");
        return -1;
    }

    snv::GLBackend::Init();
    snv::GLBackend::SetViewport(0, 0, Window::DefaultWindowWidth, Window::DefaultWindowHeight);
    snv::GLBackend::SetClearColor(0.1f, 0.1f, 0.80f, 1.0f);
    snv::GLBackend::EnableDepthTest();
    snv::GLBackend::SetDepthFunction(snv::GLDepthFunction::Less);

    const auto vertexSource = LoadShaderFromFile(k_VertexSourcePath);
    const auto fragmentSource = LoadShaderFromFile(k_FragmentSourcePath);
    snv::GLShader triangleShader(vertexSource.get(), fragmentSource.get());
    triangleShader.Bind();


    // Triangle buffer
    f32 vertices[] =
    {
        // positions        // colors
         0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
         0.0f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f
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


    snv::GameObject gameObject;
    const auto& transform = gameObject.GetComponent<snv::Transform>();
    LOG_INFO(glm::to_string(transform.GetTransform()));


    const i32 maxFPS = 60;
    const auto maxPeriod = 1.0 / maxFPS;
    auto startTime = std::chrono::high_resolution_clock::now().time_since_epoch();
    while (!Window::IsShouldBeClosed())
    {

        //TODO FPS lock
        auto currentTime = std::chrono::high_resolution_clock::now().time_since_epoch();
        auto elapsed = 1.0 / (currentTime - startTime).count();
        //LOG_INFO("CORRECT");
        startTime = currentTime;
        //snv::InputSystem::Update(elapsed);
        Update(elapsed);
        Render();
    }

    LOG_TRACE("SuperNova-Engine Shutdown");

    Window::Terminate();

    return 0;
}
