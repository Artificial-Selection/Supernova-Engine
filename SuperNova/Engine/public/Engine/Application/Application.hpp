#pragma once

#include <vector>


namespace snv
{

class IApplicationLayer;


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
