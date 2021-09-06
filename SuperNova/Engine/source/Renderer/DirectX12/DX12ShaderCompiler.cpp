#include <Engine/Renderer/DirectX12/DX12ShaderCompiler.hpp>


namespace snv
{

DX12ShaderCompiler::DX12ShaderCompiler()
{
    // NOTE(v.matushkin): DxcCreateInstance2?
    DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(m_compiler.GetAddressOf()));
    DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(m_utils.GetAddressOf()));
}


DX12ShaderBytecode DX12ShaderCompiler::CompileShader(LPCWSTR profile, std::span<const char> shaderSource)
{
    // https://developer.nvidia.com/dx12-dos-and-donts
    // Use the /all_resources_bound / D3DCOMPILE_ALL_RESOURCES_BOUND compile flag if possible
    // This allows for the compiler to do a better job at optimizing texture accesses. We have
    // seen frame rate improvements of > 1 % when toggling this flag on.
    LPCWSTR arguments[] = {
        DXC_ARG_ALL_RESOURCES_BOUND
    };

    Microsoft::WRL::ComPtr<IDxcCompilerArgs> dxcCompilerArgs;
    auto hr = m_utils->BuildArguments(nullptr, L"main", profile, arguments, ARRAYSIZE(arguments), nullptr, 0, dxcCompilerArgs.GetAddressOf());

    DxcBuffer dxcBuffer = {
        .Ptr      = shaderSource.data(),
        .Size     = shaderSource.size_bytes(),
        .Encoding = 0, //NOTE(v.matushkin): ???
    };

    Microsoft::WRL::ComPtr<IDxcResult> dxcResult;
    hr = m_compiler->Compile(&dxcBuffer, dxcCompilerArgs->GetArguments(), dxcCompilerArgs->GetCount(), nullptr, IID_PPV_ARGS(dxcResult.GetAddressOf()));

    HRESULT status;
    dxcResult->GetStatus(&status);
    if (FAILED(status))
    {
        Microsoft::WRL::ComPtr<IDxcBlobEncoding> dxcErrors;
        dxcResult->GetErrorBuffer(dxcErrors.GetAddressOf());
        OutputDebugStringA((LPSTR)dxcErrors->GetBufferPointer());
    }

    Microsoft::WRL::ComPtr<IDxcBlob> dxcBlob;
    hr = dxcResult->GetResult(dxcBlob.GetAddressOf());

    auto bytecodeLength = dxcBlob->GetBufferSize();
    auto bytecode       = std::make_unique<ui8[]>(bytecodeLength);
    std::memcpy(bytecode.get(), dxcBlob->GetBufferPointer(), bytecodeLength);

    return { .Bytecode = std::move(bytecode), .Length = bytecodeLength};
}

} // namespace snv
