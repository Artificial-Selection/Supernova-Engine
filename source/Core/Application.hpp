#pragma once
#include <string>
#include <memory>
#include "Window.hpp"

extern int main();

namespace SuperNova
{
    class Application
    {
    public:
        Application(std::string name = std::string());
        Application() = delete;


    private:
        void Init();
        void Run();

    private:
        bool m_shouldBeClosed;
        std::unique_ptr<snv::Window> m_window;
        friend int ::main();
    };

}
