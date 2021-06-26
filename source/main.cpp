//
// Created by Devilast on 08.09.2020.
//
#include <Core/Log.hpp>
#include <Library/ObservableField.h>
#include <Renderer/OpenGL/GLBackend.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <chrono>


constexpr ui32 k_ScreenWidth = 800;
constexpr ui32 k_ScreenHeight = 600;


void RequestHandler(i32 value)
{
    LOG_INFO( "Request proceed {}", value );
}

void OnValueChanged(i32 newValue) {
    LOG_INFO( "value changed" );
}

void TestObservableField()
{
    ObservableField intValue(50);
    intValue.OnChange += OnValueChanged;
    intValue += 30;
}

void TestSecondRequest(i32 value)
{
    LOG_INFO( "TestSecondRequest" );
}

void ProcessInput( GLFWwindow* window )
{
    glfwPollEvents();

    if (glfwGetKey( window, GLFW_KEY_ESCAPE ) == GLFW_PRESS) {
        glfwSetWindowShouldClose( window, true );
    }
}

void Render( GLFWwindow* window )
{
    // GG WP worth it
    auto bufferBitMask = static_cast<GLBufferBit>((ui32)GLBufferBit::Color | (ui32)GLBufferBit::Depth);
    GLBackend::Clear( bufferBitMask );
    glfwSwapBuffers( window );
}

void Update( f32 deltaTime )
{
    //TODO Implement
}

int main()
{
    //Log::Init( spdlog::level::trace );
    LOG_TRACE( "SuperNova-Engine Init" );

    glfwInit();
    glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
    glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 6 );
    glfwWindowHint( GLFW_OPENGL_CORE_PROFILE, GLFW_OPENGL_CORE_PROFILE );
    glfwWindowHint( GLFW_RESIZABLE, false );
#ifdef SNV_ENABLE_DEBUG
    glfwWindowHint( GLFW_OPENGL_DEBUG_CONTEXT, true );
#endif

    GLFWwindow* window = glfwCreateWindow( k_ScreenWidth, k_ScreenHeight, "SuperNova-Engine", nullptr, nullptr );
    if (window == nullptr) {
        LOG_CRITICAL( "Failed to create GLFW window" );
        return -1;
    }

    glfwMakeContextCurrent( window );

    if (!gladLoadGLLoader( (GLADloadproc)glfwGetProcAddress )) {
        LOG_CRITICAL( "Failed to initialize GLAD" );
        return -1;
    }

    GLBackend::Init();
    GLBackend::SetViewport( 0, 0, k_ScreenWidth, k_ScreenHeight );
    GLBackend::SetClearColor( 0.1f, 0.1f, 0.80f, 1.0f );
    GLBackend::EnableDepthTest();
    GLBackend::SetDepthFunction( GLDepthFunction::Less );

    const i32 maxFPS = 60;
    const auto maxPeriod = 1.0 / maxFPS;
    // NOTE: glfwGetTime() ?
    auto startTime = std::chrono::high_resolution_clock::now().time_since_epoch();
    TestObservableField();

    while (!glfwWindowShouldClose( window )) {
        //TODO FPS lock
        auto currentTime = std::chrono::high_resolution_clock::now().time_since_epoch();
        auto elapsed = 1.0 / (currentTime - startTime).count();
        LOG_INFO( "current lag is {}", elapsed );
        startTime = currentTime;

        ProcessInput(window);
        Update(elapsed);
        Render(window);
    }

    LOG_TRACE( "SuperNova-Engine Shutdown" );

    glfwTerminate();

    return 0;
}

