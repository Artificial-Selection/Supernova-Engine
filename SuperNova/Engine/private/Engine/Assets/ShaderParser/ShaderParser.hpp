#pragma once

#include <Engine/Renderer/RenderTypes.hpp>

#include <memory>
#include <string>


// TODO(v.matushkin): ShaderParser should be in the Editor ? Engine should use only binary format to create a shader?


namespace snv
{

class Lexer; // NOTE(v.matushkin): Forward declaration only to not include Lexer.h, with modules should be unnecessary


class ShaderParser
{
public:
    ShaderParser();
    ~ShaderParser();

    [[nodiscard]] ShaderDesc Parse(const std::string& shaderSource);

private:
    [[nodiscard]] std::string ParseShaderName();
                  void        ParseShaderBody(ShaderDesc& shaderDesc);
    [[nodiscard]] std::string ParseShaderStage() const;

    // ShaderState
    [[nodiscard]] ShaderState          ParseShaderState();
    [[nodiscard]] BlendState           ParseBlendStateValue();
    [[nodiscard]] CullMode             ParseCullModeValue();
    [[nodiscard]] DepthCompareFunction ParseDepthCompareFunctionValue();

private:
    std::unique_ptr<Lexer> m_lexer;
};

} // namespace snv
