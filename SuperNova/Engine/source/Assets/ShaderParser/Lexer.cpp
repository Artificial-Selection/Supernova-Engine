#include <Engine/Assets/ShaderParser/Lexer.hpp>
#include <Engine/Assets/ShaderParser/ShaderParserUtils.hpp>
#include <Engine/Utils/EnumStrings.hpp>

#include <unordered_map>


bool isEndOfLine(char c)  { return c == '\n' || c == '\r'; }
bool isWhitespace(char c) { return c == ' ' || c == '\t' || c == '\v' || c == '\f' || isEndOfLine(c); }
bool isAlpha(char c)      { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); };
bool isDigit(char c)      { return c >= '0' && c <= '9'; }


namespace snv
{

Lexer::Lexer(const std::string& text)
    : m_peekToken(std::nullopt)
    , m_textCurrent(text.begin())
    , m_textEnd(text.end())
    , m_line(1)
    , m_column(1)
{}


Token Lexer::NextToken()
{
    if (m_peekToken.has_value())
    {
        const auto token = m_peekToken.value();
        m_peekToken.reset();

        return token;
    }

    SkipWhitespace();

    if (m_textCurrent == m_textEnd)
    {
        return Token
        {
            .Text   = std::string_view(m_textCurrent, m_textEnd),
            .Line   = m_line,
            .Column = m_column,
            .Type   = TokenType::EndOfFile,
        };
    }

    const auto currentChar = *m_textCurrent;

    switch (currentChar)
    {
    case '{':  return Atom(TokenType::OpenBrace);
    case '}':  return Atom(TokenType::CloseBrace);
    case ',':  return Atom(TokenType::Comma);
    case '"':  return String();
    };

    // Identifier/keywords
    if (isAlpha(currentChar))
    {
        return Identifier();
    }
    // NOTE(v.matushkin): What if it's repeated '-' or '-[^0-9]+'
    else if (isDigit(currentChar) || currentChar == '-')
    {
        return Number();
    }

    return Atom(TokenType::Undefined);
}

Token Lexer::PeekNextToken()
{
    if (m_peekToken.has_value())
    {
        return m_peekToken.value();
    }

    m_peekToken = NextToken();

    return m_peekToken.value();
}

void Lexer::Advance()
{
    if (m_peekToken.has_value())
    {
        m_peekToken.reset();
    }
}

Token Lexer::ExpectToken(TokenType tokenType)
{
    const auto token = NextToken();
    if (token.Type != tokenType)
    {
        throw MakeError::UnexpectedTokenType(token, tokenType);
    }

    return token;
}

Token Lexer::ExpectToken(TokenType tokenType1, TokenType tokenType2)
{
    const auto token = NextToken();
    if (token.Type != tokenType1 && token.Type != tokenType2)
    {
        throw MakeError::UnexpectedTokenType(token, tokenType1, tokenType2);
    }

    return token;
}


// TODO(v.matushkin): <HeterogeneousLookup>
// TODO(v.matushkin): return TokenType::Identifier or TokenType::Undefined?
TokenType Lexer::StringToTokenType(const std::string& word)
{
    static const std::unordered_map<std::string, TokenType> stringToTokenType
    {
        {TokenString::Shader,                      TokenType::Shader},
        {TokenString::State,                       TokenType::State},
        {TokenString::Blend,                       TokenType::Blend},
        {TokenString::Cull,                        TokenType::Cull},
        {TokenString::DepthTest,                   TokenType::DepthTest},
        {TokenString::DepthCompareFunction,        TokenType::DepthCompareFunction},
        {TokenString::Vertex,                      TokenType::Vertex},
        {TokenString::Fragment,                    TokenType::Fragment},

        //- Values
        {"On",                                     TokenType::Value_On},
        {"Off",                                    TokenType::Value_Off},

        {BlendModeString::Zero,                    TokenType::Value_BlendMode},
        {BlendModeString::One,                     TokenType::Value_BlendMode},
        {BlendModeString::DstColor,                TokenType::Value_BlendMode},
        {BlendModeString::SrcColor,                TokenType::Value_BlendMode},
        {BlendModeString::OneMinusDstColor,        TokenType::Value_BlendMode},
        {BlendModeString::SrcAlpha,                TokenType::Value_BlendMode},
        {BlendModeString::OneMinusSrcColor,        TokenType::Value_BlendMode},
        {BlendModeString::DstAlpha,                TokenType::Value_BlendMode},
        {BlendModeString::OneMinusDstAlpha,        TokenType::Value_BlendMode},
        {BlendModeString::ScrAlphaSaturate,        TokenType::Value_BlendMode},
        {BlendModeString::OneMinusSrcAlpha,        TokenType::Value_BlendMode},

        {CullModeString::Front,                    TokenType::Value_CullMode},
        {CullModeString::Back,                     TokenType::Value_CullMode},

        {DepthCompareFunctionString::Never,        TokenType::Value_DepthCompareFunction},
        {DepthCompareFunctionString::Less,         TokenType::Value_DepthCompareFunction},
        {DepthCompareFunctionString::Equal,        TokenType::Value_DepthCompareFunction},
        {DepthCompareFunctionString::LessEqual,    TokenType::Value_DepthCompareFunction},
        {DepthCompareFunctionString::Greater,      TokenType::Value_DepthCompareFunction},
        {DepthCompareFunctionString::NotEqual,     TokenType::Value_DepthCompareFunction},
        {DepthCompareFunctionString::GreaterEqual, TokenType::Value_DepthCompareFunction},
        {DepthCompareFunctionString::Always,       TokenType::Value_DepthCompareFunction},
    };

    if (auto mapIterator = stringToTokenType.find(word); mapIterator != stringToTokenType.end())
    {
        return mapIterator->second;
    }

    return TokenType::Identifier;
}


void Lexer::SkipWhitespace()
{
    while (m_textCurrent != m_textEnd)
    {
        // Check if it is a pure whitespace first.
        if (isWhitespace(*m_textCurrent))
        {
            // Handle change of line
            if (isEndOfLine(*m_textCurrent))
            {
                m_line++;
                m_column = -1;
            }
            m_textCurrent++;
            m_column++;
            // NOTE: Hack, handle \r\n end of line. Gonna break if EndOfStream?
            if (isEndOfLine(*m_textCurrent))
            {
                m_column = -1;
                m_textCurrent++;
                m_column++;
            }
        }
        // Check for single line comments ("//")
        // NOTE: what if there is position[0] == '/' at the end? what if there is no position[1]?
        else if (*m_textCurrent == '/' && *(m_textCurrent + 1) == '/')
        {
            m_textCurrent += 2;
            m_column += 2;

            while (m_textCurrent != m_textEnd && isEndOfLine(*m_textCurrent) == false)
            {
                m_textCurrent++;
                m_column++;
            }
        }
        // Check for c-style multi-lines comments
        // NOTE: same as above 'else if'
        else if (*m_textCurrent == '/' && *(m_textCurrent + 1) == '*')
        {
            m_textCurrent += 2;
            m_column += 2;
            // NOTE: invert?
            while ((*m_textCurrent == '*' && *(m_textCurrent + 1) == '/') == false)
            {
                // Handle change of line
                if (isEndOfLine(*m_textCurrent))
                {
                    m_line++;
                    m_column = -1;
                }
                m_textCurrent++;
                m_column++;
            }
            // NOTE: Will this work?
            if (*m_textCurrent == '*')
            {
                m_textCurrent += 2;
                m_column += 2;
            }
        }
        else
        {
            break;
        }
    }
}


Token Lexer::Atom(TokenType tokenType)
{
    const auto tokenBegin = m_textCurrent++;
    const auto tokenColumn = m_column++;

    return Token
    {
        .Text   = std::string_view(tokenBegin, m_textCurrent),
        .Line   = m_line,
        .Column = tokenColumn,
        .Type   = tokenType,
    };
}

Token Lexer::Identifier()
{
    const auto tokenBegin = m_textCurrent++;
    const auto tokenColumn = m_column++;

    while (isAlpha(*m_textCurrent) || isDigit(*m_textCurrent) || *m_textCurrent == '_')
    {
        m_textCurrent++;
        m_column++;
    }

    return Token
    {
        .Text   = std::string_view(tokenBegin, m_textCurrent),
        .Line   = m_line,
        .Column = tokenColumn,
        .Type   = StringToTokenType(std::string(tokenBegin, m_textCurrent)), // TODO(v.matushkin): <HeterogeneousLookup>
    };
}

Token Lexer::Number()
{
    const auto tokenBegin = m_textCurrent;
    const auto tokenColumn = m_column;

    // Parse the following literals:
    // 58, -58, 0.003, 4e2, 123.456e-67, 0.1E4f

    // 1. Sign detection
    i32 sign = 1;
    if (*m_textCurrent == '-')
    {
        sign = -1;
        m_textCurrent++;
        m_column++;
    }

    // 2. Heading zeros (00.003)
    while (*m_textCurrent == '0')
    {
        m_textCurrent++;
        m_column++;
    }

    // 3. Decimal part (until the point)
    i32 decimalPart = 0;
    if (isDigit(*m_textCurrent))
    {
        decimalPart = *m_textCurrent - '0';
        m_textCurrent++;
        m_column++;

        while (*m_textCurrent != '.' && isDigit(*m_textCurrent))
        {
            decimalPart = decimalPart * 10 + *m_textCurrent - '0';
            m_textCurrent++;
            m_column++;
        }
    }

    // 4. Fractional part
    i32 fractionalPart    = 0;
    i32 fractionalDivisor = 1;

    if (*m_textCurrent == '.')
    {
        m_textCurrent++;
        m_column++;

        while (isDigit(*m_textCurrent))
        {
            fractionalPart = fractionalPart * 10 + *m_textCurrent - '0';
            fractionalDivisor *= 10;
            m_textCurrent++;
            m_column++;
        }
    }

    const auto tokenEnd = m_textCurrent;

    // 5. Exponent (if present)
    if (*m_textCurrent == 'e' || *m_textCurrent == 'E')
    {
        m_textCurrent++;
        m_column++;
    }

    return Token
    {
        .Text   = std::string_view(tokenBegin, tokenEnd),
        .Line   = m_line,
        .Column = tokenColumn,
        .Type   = TokenType::Value_String,
    };
}

Token Lexer::String()
{
    // Skip '"' character
    const auto tokenBegin = ++m_textCurrent;
    const auto tokenColumn = ++m_column;

    while (m_textCurrent != m_textEnd && *m_textCurrent != '"')
    {
        if (*m_textCurrent == '\\' && *(m_textCurrent + 1) == '\0')
        {
            m_textCurrent++;
            m_column++;
        }
        m_textCurrent++;
        m_column++;
    }

    const auto tokenEnd = m_textCurrent;

    // NOTE: what if string didn't end with '"' ?
    if (*m_textCurrent == '"')
    {
        m_textCurrent++;
        m_column++;
    }

    return Token
    {
        .Text   = std::string_view(tokenBegin, tokenEnd),
        .Line   = m_line,
        .Column = tokenColumn,
        .Type   = TokenType::Value_String,
    };
}

} // namespace snv
