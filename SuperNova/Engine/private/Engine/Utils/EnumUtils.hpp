#pragma once

#include <Engine/Core/Core.hpp>
#include <Engine/Core/Assert.hpp>
#include <Engine/Renderer/RenderTypes.hpp>

#include <string>


// TODO(v.matushkin): std::string -> std::string_view ?


namespace snv
{
// Engine/Assets/ShaderParser/Lexer.hpp
enum class TokenType : ui8;
}

namespace snv::EnumUtils
{

// Engine/Assets/ShaderParser/Lexer.hpp
[[nodiscard]] std::string ToString(TokenType tokenType);
// Engine/Renderer/RenderTypes.hpp
[[nodiscard]] std::string ToString(CullMode cullMode);
[[nodiscard]] std::string ToString(CompareFunction CompareFunction);
[[nodiscard]] std::string ToString(BlendOp blendOp);
[[nodiscard]] std::string ToString(BlendFactor blendFactor);


template<typename T>
[[nodiscard]] T FromString(const std::string& str)
{
    // NOTE(v.matushkin): With 'VS 2022 17.1.0 Preview 3.0' update microsoft broke this static_assert for me.
    //  Can't find the reason, so I'll just wait for it to go away.
    //  Remove #include <Engine/Core/Assert.hpp> when this will be fixed
    // static_assert(false, "No, I don't think so");
    SNV_ASSERT(false, str);
}

// Engine/Renderer/RenderTypes.hpp
template<> [[nodiscard]] CullMode        FromString<CullMode>       (const std::string& str);
template<> [[nodiscard]] CompareFunction FromString<CompareFunction>(const std::string& str);
template<> [[nodiscard]] BlendOp         FromString<BlendOp>        (const std::string& str);
template<> [[nodiscard]] BlendFactor     FromString<BlendFactor>    (const std::string& str);

} // namespace snv::EnumUtils
