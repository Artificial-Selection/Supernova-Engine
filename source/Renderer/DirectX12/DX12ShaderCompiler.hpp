#pragma once

#include <Core/Core.hpp>

#include <wrl/client.h>

#include <memory>
#include <span>


struct IDxcCompiler3;
struct IDxcUtils;


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

    [[nodiscard]] DX12ShaderBytecode CompileShader(LPCWSTR profile, std::span<const char> shaderSource);

private:
    Microsoft::WRL::ComPtr<IDxcCompiler3> m_compiler;
    Microsoft::WRL::ComPtr<IDxcUtils>     m_utils;
};

} // namespace snv
