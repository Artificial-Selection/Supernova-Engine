#include <Engine/Application/Application.hpp>
#include <Engine/Core/Core.hpp>
#include <Engine/Engine.hpp>
#include <Engine/EngineSettings.hpp>

#include <Editor/Editor.hpp>


i32 main()
{
    snv::ApplicationSettings::WindowSettings = {
        .Title  = "SuperNova-Engine",
        .Width  = 1920,
        .Height = 1080,
    };

    snv::EngineSettings::GraphicsSettings = {
        .RenderWidth     = 1280,
        .RenderHeight    = 1000,
        .GraphicsApi     = snv::GraphicsApi::DirectX11,
        .SwapchainFormat = snv::RenderTextureFormat::BGRA32,
    };

    snv::Application app;
    app.AddLayer(new snv::Engine());
    app.AddLayer(new snv::Editor());
    app.Run();

    return 0;
}
