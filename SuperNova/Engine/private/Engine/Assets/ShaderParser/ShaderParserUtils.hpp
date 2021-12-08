#pragma once

#include <Engine/Core/Core.hpp>
#include <Engine/Assets/ShaderParser/ShaderParserError.hpp>


namespace snv
{

enum class TokenType : ui8;
struct Token;


namespace MakeError
{

    ShaderParserError UnexpectedTokenType(const Token& receivedToken);
    ShaderParserError UnexpectedTokenType(const Token& receivedToken, TokenType expectedTokenType);
    ShaderParserError UnexpectedTokenType(const Token& receivedToken, TokenType expectedTokenType1, TokenType expectedTokenType2);
    ShaderParserError UnexpectedEndOfFile(const Token& receivedToken);

} // namespace MakeError

} // namespace snv
