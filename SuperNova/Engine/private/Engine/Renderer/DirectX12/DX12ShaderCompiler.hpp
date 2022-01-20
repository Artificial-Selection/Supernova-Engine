#pragma once

#include <wrl/client.h>
// NOTE(v.matushkin): I think this is fixed?
//  <dxcapi.h> shouldn't be included here, but compiler cries about IDxcCompiler3/IDxcUtils forward declarations.
//  DX12ShaderCompiler shouldn't be a member of DX12Backend.
//  This will be fixed when I make ShaderCompiler class, which will do glsl/hlsl compilation for all Backends.
//  *Backend will then just get some Shader blob instead of doing shader compilation

#include <string>
#include <vector>


// #include <dxc/dxcapi.h>
struct IDxcCompiler3;
struct IDxcUtils;
struct IDxcBlob;
// #include <dxc/d3d12shader.h>
struct ID3D12ShaderReflection;

struct D3D12_INPUT_ELEMENT_DESC;
struct D3D12_SHADER_BYTECODE;


namespace snv
{

struct ShaderDesc;


class DX12ShaderCompiler
{
    template<class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

public:
    // struct ShaderBuffer
    // {
    //     const void* Ptr;
    //     ui64        Size;
    // };

    // struct DX12ShaderDesc
    // {
    //     ComPtr<IDxcBlob>                      VertexBytecode;
    //     ComPtr<IDxcBlob>                      PixelBytecode;
    //     std::vector<D3D12_INPUT_ELEMENT_DESC> InputLayoutDesc;
    // };


    DX12ShaderCompiler(const ShaderDesc& shaderDesc);
    ~DX12ShaderCompiler();

    [[nodiscard]] D3D12_SHADER_BYTECODE GetVertexShaderBuffer() const;
    [[nodiscard]] D3D12_SHADER_BYTECODE GetPixelShaderBuffer() const;

    [[nodiscard]] std::vector<D3D12_INPUT_ELEMENT_DESC> GetInputLayoutDesc() const;

private:
    [[nodiscard]] IDxcBlob*               CompileShader(const std::string& shaderSource, const wchar_t* profile);
    [[nodiscard]] ID3D12ShaderReflection* CreateReflection(IDxcBlob* shaderBlob);

private:
    IDxcCompiler3*          m_compiler;
    IDxcUtils*              m_utils;
    IDxcBlob*               m_vertexShaderBlob;
    IDxcBlob*               m_pixelShaderBlob;
    ID3D12ShaderReflection* m_vertexShaderReflection;
    const bool              m_isImGuiShader;
};

} // namespace snv
