#pragma once

#include <Engine/Core/Core.hpp>

#include <string>
#include <vector>


namespace snv::VulkanShaderCompiler
{

// Values corresponds to EShLanguage
enum class ShaderType : ui32
{
    Vertex   = 0,
    Fragment = 4,
};


void Init();
void Shutdown();

std::vector<ui32> CompileShader(ShaderType shaderType, const std::string& shaderStageSource);

} // namespace snv::VulkanShaderCompiler
