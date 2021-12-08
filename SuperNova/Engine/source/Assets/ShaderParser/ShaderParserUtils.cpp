#include <Engine/Assets/ShaderParser/ShaderParserUtils.hpp>
#include <Engine/Assets/ShaderParser/Lexer.hpp>
#include <Engine/Utils/EnumUtils.hpp>

#include <string>

#include <fmt/format.h>


namespace snv
{

// NOTE(v.matushkin): Pass by value/&/&& ?
template<typename... Args>
ShaderParserError MakeFormattedError(const Token& receivedToken, const char* message, Args... args)
{
    const auto errorMessage = fmt::format(
        message + std::string("\nLine: {:d}\nColumn: {:d}"),
        args...,
        receivedToken.Line,
        receivedToken.Column
    );
    return ShaderParserError(errorMessage);
}


namespace MakeError
{

    ShaderParserError UnexpectedTokenType(const Token& receivedToken)
    {
        return receivedToken.Type == TokenType::Identifier
            ? MakeFormattedError(receivedToken, "Unexpected Identifier '{}'", receivedToken.Text)
            : MakeFormattedError(receivedToken, "Unexpected '{}'", EnumUtils::ToString(receivedToken.Type));
    }

    ShaderParserError UnexpectedTokenType(const Token& receivedToken, TokenType expectedTokenType)
    {
        return MakeFormattedError(
            receivedToken,
            "Expected '{}', got '{}'", EnumUtils::ToString(expectedTokenType), EnumUtils::ToString(receivedToken.Type)
        );
    }

    ShaderParserError UnexpectedTokenType(const Token& receivedToken, TokenType expectedTokenType1, TokenType expectedTokenType2)
    {
        return MakeFormattedError(
            receivedToken,
            "Expected '{}' or '{}', got '{}'",
            EnumUtils::ToString(expectedTokenType1), EnumUtils::ToString(expectedTokenType2), EnumUtils::ToString(receivedToken.Type)
        );
    }

    ShaderParserError UnexpectedEndOfFile(const Token& receivedToken)
    {
        return MakeFormattedError(receivedToken, "Unexpected end of file");
    }

} // namespace MakeError

} // namespace snv
