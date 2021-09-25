#pragma once

#include <Engine/Core/Core.hpp>

#include <span>
#include <vector>


namespace snv::VKShaderCompiler
{

// Values corresponds to EShLanguage
enum class ShaderType : ui32
{
    Vertex   = 0,
    Fragment = 4,
};


void Init();
void Shutdown();

std::vector<ui32> CompileShader(ShaderType shaderType, std::span<const char> shaderSource);

} // namespace snv::VKShaderCompiler
