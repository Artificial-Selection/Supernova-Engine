#pragma once

#include <Engine/Core/Core.hpp>

#include <wrl/client.h>
// NOTE(v.matushkin): <dxcapi.h> shouldn't be included here, but compiler cries about IDxcCompiler3/IDxcUtils forward declarations.
//  DX12ShaderCompiler shouldn't be a member of DX12Backend.
//  This will be fixed when I make ShaderCompiler class, which will do glsl/hlsl compilation for all Backends.
//  *Backend will then just get some Shader blob instead of doing shader compilation
#include <dxc/dxcapi.h>

#include <memory>
#include <string>


namespace snv
{

struct DX12ShaderBytecode
{
    std::unique_ptr<ui8[]> Bytecode;
    ui64                   Length;
};


class DX12ShaderCompiler
{
public:
    DX12ShaderCompiler();

    [[nodiscard]] DX12ShaderBytecode CompileShader(LPCWSTR profile, const std::string& shaderStageSource);

private:
    Microsoft::WRL::ComPtr<IDxcCompiler3> m_compiler;
    Microsoft::WRL::ComPtr<IDxcUtils>     m_utils;
};

} // namespace snv
