#include <Engine/Assets/ShaderParser/ShaderParser.hpp>
#include <Engine/Assets/ShaderParser/ShaderParserUtils.hpp>
#include <Engine/Assets/ShaderParser/Lexer.hpp>
#include <Engine/Utils/EnumUtils.hpp>


namespace snv
{

template<typename Enum>
Enum GetEnumValue(Token valueToken)
{
    return EnumUtils::FromString<Enum>(std::string(valueToken.Text));
}


ShaderParser::ShaderParser() = default;
ShaderParser::~ShaderParser() = default;


ShaderDesc ShaderParser::Parse(const std::string& shaderSource)
{
    m_lexer = std::make_unique<Lexer>(shaderSource); // NOTE(v.matushkin): Useless allocation every time Parse is called

    ShaderDesc shaderDesc = {
        .RasterizerStateDesc   = RasterizerStateDesc::Default(),
        .DepthStencilStateDesc = DepthStencilStateDesc::Default(),
        .BlendStateDesc        = BlendStateDesc::Default(),
    };

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

    bool foundShaderState   = false;
    bool foundVertexBlock   = false;
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
            ParseShaderState(shaderDesc);
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


void ShaderParser::ParseShaderState(ShaderDesc& shaderDesc)
{
    m_lexer->ExpectToken(TokenType::OpenBrace);

    bool foundCull         = false;
    bool foundDepthTest    = false;
    bool foundDepthWrite   = false;
    bool foundDepthCompare = false;
    bool foundBlendOp      = false;
    bool foundBlend        = false;

    while (true)
    {
        const auto token = m_lexer->NextToken();

        if (token.Type == TokenType::CloseBrace)
        {
            break;
        }

        switch (token.Type)
        {
        case TokenType::Cull:
            {
                if (foundCull)
                {
                    throw ShaderParserError("Found more than one Cull value");
                }
                foundCull = true;
                shaderDesc.RasterizerStateDesc.CullMode = ParseCullModeValue();
            }
            break;
        case TokenType::DepthTest:
            {
            if (foundDepthTest)
                {
                    throw ShaderParserError("Found more than one DepthTest value");
                }
                foundDepthTest = true;
                shaderDesc.DepthStencilStateDesc.DepthTestEnable = ParseDepthTestValue();
            }
            break;
        case TokenType::DepthWrite:
            {
            if (foundDepthWrite)
                {
                    throw ShaderParserError("Found more than one DepthWrite value");
                }
                foundDepthWrite = true;
                shaderDesc.DepthStencilStateDesc.DepthWriteEnable = ParseDepthWriteValue();
            }
            break;
        case TokenType::DepthCompare:
            {
                if (foundDepthCompare)
                {
                    throw ShaderParserError("Found more than one DepthCompareFunction value");
                }
                foundDepthCompare = true;
                shaderDesc.DepthStencilStateDesc.DepthCompareFunction = ParseDepthCompareValue();
            }
            break;
        case TokenType::BlendOp:
            {
                if (foundBlendOp)
                {
                    throw ShaderParserError("Found more than one BlendOp value");
                }
                foundBlendOp = true;
                ParseBlendOpValue(shaderDesc.BlendStateDesc);
            }
            break;
        case TokenType::Blend:
            {
                if (foundBlend)
                {
                    throw ShaderParserError("Found more than one Blend value");
                }
                foundBlend = true;
                ParseBlendValue(shaderDesc.BlendStateDesc);
            }
            break;

        default: throw MakeError::UnexpectedTokenType(token);
        }
    }

    if (foundBlendOp || foundBlend)
    {
        shaderDesc.BlendStateDesc.BlendMode = BlendMode::BlendOp;
    }
}


CullMode ShaderParser::ParseCullModeValue()
{
    const auto cullValueToken = m_lexer->ExpectToken(TokenType::Value_CullMode, TokenType::Value_Off);
    return EnumUtils::FromString<CullMode>(std::string(cullValueToken.Text));
}

bool ShaderParser::ParseDepthTestValue()
{
    const auto depthTestValueToken = m_lexer->ExpectToken(TokenType::Value_On, TokenType::Value_Off);
    return depthTestValueToken.Type == TokenType::Value_On;
}

bool ShaderParser::ParseDepthWriteValue()
{
    const auto depthWriteValueToken = m_lexer->ExpectToken(TokenType::Value_On, TokenType::Value_Off);
    return depthWriteValueToken.Type == TokenType::Value_On;
}

CompareFunction ShaderParser::ParseDepthCompareValue()
{
    const auto depthCompareValueToken = m_lexer->ExpectToken(TokenType::Value_CompareFunction);
    return EnumUtils::FromString<CompareFunction>(std::string(depthCompareValueToken.Text));
}

void ShaderParser::ParseBlendOpValue(BlendStateDesc& blendStateDesc)
{
    blendStateDesc.ColorBlendOp = GetEnumValue<BlendOp>(m_lexer->ExpectToken(TokenType::Value_BlendOp));

    if (m_lexer->PeekNextToken().Type == TokenType::Comma)
    {
        m_lexer->Advance();

        blendStateDesc.AlphaBlendOp = GetEnumValue<BlendOp>(m_lexer->ExpectToken(TokenType::Value_BlendOp));
    }
    else
    {
        blendStateDesc.AlphaBlendOp = blendStateDesc.ColorBlendOp;
    }
}

void ShaderParser::ParseBlendValue(BlendStateDesc& blendStateDesc)
{
    blendStateDesc.ColorSrcBlendFactor = GetEnumValue<BlendFactor>(m_lexer->ExpectToken(TokenType::Value_BlendFactor));
    blendStateDesc.ColorDstBlendFactor = GetEnumValue<BlendFactor>(m_lexer->ExpectToken(TokenType::Value_BlendFactor));

    if (m_lexer->PeekNextToken().Type == TokenType::Comma)
    {
        m_lexer->Advance();

        blendStateDesc.AlphaSrcBlendFactor = GetEnumValue<BlendFactor>(m_lexer->ExpectToken(TokenType::Value_BlendFactor));
        blendStateDesc.AlphaDstBlendFactor = GetEnumValue<BlendFactor>(m_lexer->ExpectToken(TokenType::Value_BlendFactor));
    }
    else
    {
        blendStateDesc.AlphaSrcBlendFactor = blendStateDesc.ColorSrcBlendFactor;
        blendStateDesc.AlphaDstBlendFactor = blendStateDesc.ColorDstBlendFactor;
    }
}

} // namespace snv
