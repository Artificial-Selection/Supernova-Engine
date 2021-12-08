#include <Engine/Assets/ShaderParser/ShaderParser.hpp>
#include <Engine/Assets/ShaderParser/ShaderParserUtils.hpp>
#include <Engine/Assets/ShaderParser/Lexer.hpp>
#include <Engine/Utils/EnumUtils.hpp>


namespace snv
{

ShaderParser::ShaderParser() = default;
ShaderParser::~ShaderParser() = default;


ShaderDesc ShaderParser::Parse(const std::string& shaderSource)
{
    m_lexer = std::make_unique<Lexer>(shaderSource); // NOTE(v.matushkin): Useless allocation every time Parse is called

    ShaderDesc shaderDesc;
    shaderDesc.Name = ParseShaderName();
    ParseShaderBody(shaderDesc);
    m_lexer->ExpectToken(TokenType::EndOfFile);

    return shaderDesc;
}


std::string ShaderParser::ParseShaderName()
{
    m_lexer->ExpectToken(TokenType::Shader);
    const auto shaderNameToken = m_lexer->ExpectToken(TokenType::Value_String);

    return std::string(shaderNameToken.Text);
}

void ShaderParser::ParseShaderBody(ShaderDesc& shaderDesc)
{
    m_lexer->ExpectToken(TokenType::OpenBrace);

    bool foundShaderState = false;
    bool foundVertexBlock = false;
    bool foundFragmentBlock = false;

    while (true)
    {
        const auto token = m_lexer->NextToken();

        if (token.Type == TokenType::CloseBrace)
        {
            break;
        }
        if (token.Type == TokenType::EndOfFile)
        {
            throw MakeError::UnexpectedEndOfFile(token);
        }

        if (token.Type == TokenType::State)
        {
            if (foundShaderState)
            {
                throw ShaderParserError("Found more than one State block");
            }
            foundShaderState = true;
            shaderDesc.State = ParseShaderState();
        }
        else if (token.Type == TokenType::Vertex)
        {
            if (foundVertexBlock)
            {
                throw ShaderParserError("Found more than one Vertex shader stage");
            }
            foundVertexBlock = true;
            shaderDesc.VertexSource = ParseShaderStage();
        }
        else if (token.Type == TokenType::Fragment)
        {
            if (foundFragmentBlock)
            {
                throw ShaderParserError("Found more than one Fragment shader stage");
            }
            foundFragmentBlock = true;
            shaderDesc.FragmentSource = ParseShaderStage();
        }
        else
        {
            throw MakeError::UnexpectedTokenType(token);
        }
    }

    if (foundVertexBlock == false)
    {
        throw ShaderParserError("Vertex shader stage was not defined");
    }
    if (foundFragmentBlock == false)
    {
        throw ShaderParserError("Fragment shader stage was not defined");
    }
}

std::string ShaderParser::ParseShaderStage() const
{
    Token token = m_lexer->ExpectToken(TokenType::OpenBrace);
    auto* shaderStageBegin = token.Text.data() + 1; // NOTE(v.matushkin): Hack, do not include '{' in the shader code

    ui32 openBraces = 1;

    while (openBraces)
    {
        token = m_lexer->NextToken();

        if (token.Type == TokenType::EndOfFile)
        {
            throw MakeError::UnexpectedEndOfFile(token);
        }

        if (token.Type == TokenType::OpenBrace)
        {
            openBraces++;
        }
        else if (token.Type == TokenType::CloseBrace)
        {
            openBraces--;
        }
    }

    return std::string(shaderStageBegin, token.Text.data());
}


ShaderState ShaderParser::ParseShaderState()
{
    auto shaderState = ShaderState::Default();

    m_lexer->ExpectToken(TokenType::OpenBrace);

    while (true)
    {
        const auto token = m_lexer->NextToken();

        if (token.Type == TokenType::CloseBrace)
        {
            break;
        }

        switch (token.Type)
        {
        case TokenType::Blend:                shaderState.BlendState           = ParseBlendStateValue();           break;
        case TokenType::Cull:                 shaderState.CullMode             = ParseCullModeValue();             break;
        case TokenType::DepthCompareFunction: shaderState.DepthCompareFunction = ParseDepthCompareFunctionValue(); break;

        default: throw MakeError::UnexpectedTokenType(token);
        }
    }

    return shaderState;
}

BlendState ShaderParser::ParseBlendStateValue()
{
    BlendState blendState;
    blendState.ColorSrcBlendMode = EnumUtils::FromString<BlendMode>(std::string(m_lexer->ExpectToken(TokenType::Value_BlendMode).Text));
    blendState.ColorDstBlendMode = EnumUtils::FromString<BlendMode>(std::string(m_lexer->ExpectToken(TokenType::Value_BlendMode).Text));

    if (m_lexer->PeekNextToken().Type == TokenType::Comma)
    {
        m_lexer->Advance();

        blendState.AlphaSrcBlendMode = EnumUtils::FromString<BlendMode>(std::string(m_lexer->ExpectToken(TokenType::Value_BlendMode).Text));
        blendState.AlphaDstBlendMode = EnumUtils::FromString<BlendMode>(std::string(m_lexer->ExpectToken(TokenType::Value_BlendMode).Text));
    }
    else
    {
        blendState.AlphaSrcBlendMode = blendState.ColorSrcBlendMode;
        blendState.AlphaDstBlendMode = blendState.ColorDstBlendMode;
    }

    return blendState;
}

CullMode ShaderParser::ParseCullModeValue()
{
    const auto token = m_lexer->ExpectToken(TokenType::Value_CullMode, TokenType::Value_Off);
    return EnumUtils::FromString<CullMode>(std::string(token.Text));
}

DepthCompareFunction ShaderParser::ParseDepthCompareFunctionValue()
{
    const auto token = m_lexer->ExpectToken(TokenType::Value_DepthCompareFunction);
    return EnumUtils::FromString<DepthCompareFunction>(std::string(token.Text));
}

} // namespace snv
