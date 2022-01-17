#pragma once

#include <Engine/Core/Core.hpp>

#include <string>
#include <vector>


struct ID3D10Blob;
typedef ID3D10Blob ID3DBlob;
struct ID3D11ShaderReflection;

struct D3D11_INPUT_ELEMENT_DESC;


namespace snv
{

struct ShaderDesc;


class DX11ShaderCompiler
{
public:
    struct ShaderBuffer
    {
        const void* Ptr;
        ui64        Size;
    };


    DX11ShaderCompiler(const ShaderDesc& shaderDesc);
    ~DX11ShaderCompiler();

    [[nodiscard]] ShaderBuffer GetVertexShaderBuffer() const;
    [[nodiscard]] ShaderBuffer GetPixelShaderBuffer() const;

    [[nodiscard]] std::vector<D3D11_INPUT_ELEMENT_DESC> GetInputLayoutDesc() const;

private:
    [[nodiscard]] static  ID3DBlob* CompileShader(
        const std::string& shaderStageSource,
        const char* shaderName,
        const char* shaderTarget
    );

private:
    ID3DBlob*               m_vertexShaderBlob;
    ID3DBlob*               m_pixelShaderBlob;
    ID3D11ShaderReflection* m_vertexShaderReflection;
    const bool              m_isImGuiShader;
};

} // namespace snv
