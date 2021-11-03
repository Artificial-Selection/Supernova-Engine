#pragma once

#include <Engine/Core/Core.hpp>


namespace snv
{

enum class GraphicsApi : ui8
{
    OpenGL,
    Vulkan,
#ifdef SNV_PLATFORM_WINDOWS
    DirectX11,
    DirectX12
#endif
};


struct ApplicationSettings
{
    struct WindowSettings
    {
        const char* Title;
        ui32        Width;
        ui32        Height;
    };

    static inline WindowSettings WindowSettings;
};

// TODO(v.matushkin): Use this settings in DX11/DX12/Vulkan backends
struct EngineSettings
{
    struct GraphicsSettings
    {
        GraphicsApi GraphicsApi;
        ui32        RenderWidth;
        ui32        RenderHeight;
    };

    static inline GraphicsSettings GraphicsSettings;
};

} // namespace snv
