#pragma once

#include <wrl/client.h>
// NOTE(v.matushkin): <dxcapi.h> shouldn't be included here, but compiler cries about IDxcCompiler3/IDxcUtils forward declarations.
//  DX12ShaderCompiler shouldn't be a member of DX12Backend.
//  This will be fixed when I make ShaderCompiler class, which will do glsl/hlsl compilation for all Backends.
//  *Backend will then just get some Shader blob instead of doing shader compilation
#include <dxc/dxcapi.h>
// #include <dxc/d3d12shader.h>

#include <string>


namespace snv
{

struct ShaderDesc;


class DX12ShaderCompiler
{
    template<class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

public:
    struct DX12ShaderDesc
    {
        ComPtr<IDxcBlob> VertexBytecode;
        ComPtr<IDxcBlob> PixelBytecode;
    };


    DX12ShaderCompiler();

    [[nodiscard]] DX12ShaderDesc CompileShader(const ShaderDesc& shaderDesc);

private:
    [[nodiscard]] ComPtr<IDxcBlob> CompileShaderStage(const wchar_t* profile, const std::string& shaderStageSource);
    // [[nodiscard]] ID3D12ShaderReflection* CreateReflection(IDxcBlob* shaderBlob);

private:
    ComPtr<IDxcCompiler3> m_compiler;
    ComPtr<IDxcUtils>     m_utils;
};

} // namespace snv
