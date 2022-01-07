#pragma once

#include <Engine/Core/Core.hpp>
#include <Engine/Renderer/RenderTypes.hpp>


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

    inline static WindowSettings WindowSettings;
};

// TODO(v.matushkin): Use this settings in DX11/DX12/Vulkan backends
struct EngineSettings
{
    struct GraphicsSettings
    {
        ui32                RenderWidth;
        ui32                RenderHeight;
        GraphicsApi         GraphicsApi;
        RenderTextureFormat SwapchainFormat;
    };

    inline static GraphicsSettings GraphicsSettings;
};

} // namespace snv
