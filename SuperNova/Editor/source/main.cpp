#include <Engine/Application/Application.hpp>
#include <Engine/Core/Core.hpp>
#include <Engine/Engine.hpp>

#include <Editor/Editor.hpp>


i32 main()
{
    snv::Application app;
    app.AddLayer(new snv::Engine());
    app.AddLayer(new snv::Editor());
    app.Run();

    return 0;
}
