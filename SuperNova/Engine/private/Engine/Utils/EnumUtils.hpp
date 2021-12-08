#pragma once

#include <Engine/Core/Core.hpp>

#include <string>


// TODO(v.matushkin): std::string -> std::string_view ?


namespace snv
{

// Engine/Assets/ShaderParser/Lexer.hpp
enum class TokenType : ui8;
// Engine/Renderer/RenderTypes.hpp
enum class BlendMode : ui8;
enum class CullMode : ui8;
enum class DepthCompareFunction : ui8;


namespace EnumUtils
{

    // Engine/Assets/ShaderParser/Lexer.hpp
    [[nodiscard]] std::string ToString(TokenType tokenType);
    // Engine/Renderer/RenderTypes.hpp
    [[nodiscard]] std::string ToString(BlendMode blendMode);
    [[nodiscard]] std::string ToString(CullMode cullMode);
    [[nodiscard]] std::string ToString(DepthCompareFunction depthCompareFunction);


    template<typename T>
    [[nodiscard]] T FromString(const std::string& str)
    {
        static_assert(false, "No, I don't think so");
    }

    // Engine/Renderer/RenderTypes.hpp
    template<> [[nodiscard]] BlendMode            FromString<BlendMode>           (const std::string& str);
    template<> [[nodiscard]] CullMode             FromString<CullMode>            (const std::string& str);
    template<> [[nodiscard]] DepthCompareFunction FromString<DepthCompareFunction>(const std::string& str);

} // namespace EnumUtils

} // namespace snv
