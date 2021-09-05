#pragma once

#include <Application/IApplicationLayer.hpp>

#include <vector>


namespace snv
{

class Application
{
public:
    Application();
    ~Application();

    void AddLayer(IApplicationLayer* layer);

    void Run();

private:
    std::vector<IApplicationLayer*> m_layers;
};

} // namespace snv
