#pragma once

#include <Engine/Renderer/IImGuiRenderContext.hpp>


struct ID3D11Device;
struct ID3D11DeviceContext;


namespace snv
{

class DX11ImGuiRenderContext final : public IImGuiRenderContext
{
public:
    DX11ImGuiRenderContext(ID3D11Device* d3dDevice, ID3D11DeviceContext* d3dDeviceContext);
    ~DX11ImGuiRenderContext() override;

    void BeginFrame() override;
    void EndFrame(ImDrawData* imguiDrawData) override;
};

} // namespace snv
