#include <Engine/Application/Application.hpp>
#include <Engine/Core/Core.hpp>
#include <Engine/Engine.hpp>
#include <Engine/EngineSettings.hpp>

#include <Editor/Editor.hpp>


i32 main()
{
    snv::ApplicationSettings::WindowSettings = {
        .Title  = "SuperNova-Engine",
        .Width  = 1280,
        .Height = 720,
    };

    snv::EngineSettings::GraphicsSettings = {
        .GraphicsApi  = snv::GraphicsApi::DirectX11,
        .RenderWidth  = 800,
        .RenderHeight = 600,
    };

    snv::Application app;
    app.AddLayer(new snv::Engine());
    app.AddLayer(new snv::Editor());
    app.Run();

    return 0;
}
