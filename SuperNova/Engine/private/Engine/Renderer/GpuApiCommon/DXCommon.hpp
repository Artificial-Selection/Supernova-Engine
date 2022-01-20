#pragma once

#include <Engine/Core/Core.hpp>
#include <Engine/Renderer/RenderTypes.hpp>


enum DXGI_FORMAT;


namespace snv
{

[[nodiscard]] DXGI_FORMAT              dx_VertexAttributeFormat(VertexAttributeFormat format, ui8 dimension);
[[nodiscard]] VertexInputAttributeDesc dx_VertexInputAttribute(const char* semanticName, bool isImGuiShader);
[[nodiscard]] bool                     dx_TriangleFrontFace(TriangleFrontFace triangleFrontFace, bool isImGuiShader);

} // namespace snv
