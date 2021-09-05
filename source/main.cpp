#include <Application/Application.hpp>
#include <Core/Core.hpp>
#include <Engine/Engine.hpp>


i32 main()
{
    snv::Application app;
    app.AddLayer(new snv::Engine());
    app.Run();

    return 0;
}
