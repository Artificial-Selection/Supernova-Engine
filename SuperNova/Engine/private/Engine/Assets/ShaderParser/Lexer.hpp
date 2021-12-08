#pragma once

#include <Engine/Core/Core.hpp>

#include <optional>
#include <string>
#include <string_view>


namespace snv
{

enum class TokenType : ui8
{
    //- Identifiers
    Identifier,
    Shader,
    //-- State
    State,
    Blend,
    Cull,
    DepthTest,
    DepthCompareFunction,
    //-- Shader Stage
    Vertex,
    Fragment,

    //- Values
    Value_Number,
    Value_String,
    //-- State Value
    Value_On,
    Value_Off,
    Value_BlendMode,
    Value_CullMode,
    Value_DepthCompareFunction,

    //- Common
    OpenBrace,
    CloseBrace,
    Comma,

    EndOfFile,
    Undefined,
};


struct Token
{
    std::string_view Text;
    ui32             Line;
    ui32             Column;
    TokenType        Type;
};


class Lexer
{
public:
    Lexer(const std::string& text);
    ~Lexer() = default;

    // Skip whitespaces and get next token
    [[nodiscard]]    Token NextToken();
    [[nodiscard]]    Token PeekNextToken();
                     void  Advance();
    // Skip whitespaces and get next token, throw exception if got wrong tokenType
    [[maybe_unused]] Token ExpectToken(TokenType tokenType);
    [[maybe_unused]] Token ExpectToken(TokenType tokenType1, TokenType tokenType2); // NOTE(v.matushkin): variadic template?

private:
    [[nodiscard]] static TokenType StringToTokenType(const std::string& word);

    void SkipWhitespace();

    [[nodiscard]] Token Atom(TokenType tokenType);
    [[nodiscard]] Token Identifier();
    [[nodiscard]] Token Number();
    [[nodiscard]] Token String();

private:
    std::optional<Token>              m_peekToken;

    std::string::const_iterator       m_textCurrent;
    const std::string::const_iterator m_textEnd;

    ui32                              m_line;
    ui32                              m_column;
};

} // namespace snv
