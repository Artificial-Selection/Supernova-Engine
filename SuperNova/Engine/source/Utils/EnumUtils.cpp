#include <Engine/Utils/EnumUtils.hpp>
#include <Engine/Utils/EnumStrings.hpp>
#include <Engine/Assets/ShaderParser/Lexer.hpp>
#include <Engine/Renderer/RenderTypes.hpp>

#include <unordered_map>


namespace snv::EnumUtils
{

    // Engine/Assets/ShaderParser/Lexer.hpp

    std::string ToString(TokenType tokenType)
    {
        static const char* tokenTypeToString[]
        {
            //- Identifiers
            TokenString::Identifier,
            TokenString::Shader,
            //-- State
            TokenString::State,
            TokenString::Blend,
            TokenString::Cull,
            TokenString::DepthTest,
            TokenString::DepthCompareFunction,
            //-- Shader Stage
            TokenString::Vertex,
            TokenString::Fragment,

            //- Values
            TokenString::Value_Number,
            TokenString::Value_String,
            //-- State Value
            TokenString::Value_On,
            TokenString::Value_Off,
            TokenString::Value_BlendMode,
            TokenString::Value_CullMode,
            TokenString::Value_DepthCompareFunction,

            //- Common
            TokenString::OpenBrace,
            TokenString::CloseBrace,
            TokenString::Comma,

            TokenString::EndOfFile,
            TokenString::Undefined,
        };

        return tokenTypeToString[static_cast<ui8>(tokenType)];
    }


    // Engine/Renderer/RenderTypes.hpp

    std::string ToString(BlendMode blendMode)
    {
        static const char* blendModeToString[]
        {
            BlendModeString::Zero,
            BlendModeString::One,
            BlendModeString::DstColor,
            BlendModeString::SrcColor,
            BlendModeString::OneMinusDstColor,
            BlendModeString::SrcAlpha,
            BlendModeString::OneMinusSrcColor,
            BlendModeString::DstAlpha,
            BlendModeString::OneMinusDstAlpha,
            BlendModeString::ScrAlphaSaturate,
            BlendModeString::OneMinusSrcAlpha,
        };

        return blendModeToString[static_cast<ui8>(blendMode)];
    }

    std::string ToString(CullMode cullMode)
    {
        static const char* cullModeToString[]
        {
            CullModeString::Off,
            CullModeString::Front,
            CullModeString::Back,
        };

        return cullModeToString[static_cast<ui8>(cullMode)];
    }

    std::string ToString(DepthCompareFunction depthCompareFunction)
    {
        static const char* depthCompareFunctionToString[]
        {
            DepthCompareFunctionString::Never,
            DepthCompareFunctionString::Less,
            DepthCompareFunctionString::Equal,
            DepthCompareFunctionString::LessEqual,
            DepthCompareFunctionString::Greater,
            DepthCompareFunctionString::NotEqual,
            DepthCompareFunctionString::GreaterEqual,
            DepthCompareFunctionString::Always,
        };

        return depthCompareFunctionToString[static_cast<ui8>(depthCompareFunction)];
    }


    template<>
    BlendMode FromString<BlendMode>(const std::string& str)
    {
        static const std::unordered_map<std::string, BlendMode> stringToBlendMode
        {
            {BlendModeString::Zero,             BlendMode::Zero},
            {BlendModeString::One,              BlendMode::One},
            {BlendModeString::DstColor,         BlendMode::DstColor},
            {BlendModeString::SrcColor,         BlendMode::SrcColor},
            {BlendModeString::OneMinusDstColor, BlendMode::OneMinusDstColor},
            {BlendModeString::SrcAlpha,         BlendMode::SrcAlpha},
            {BlendModeString::OneMinusSrcColor, BlendMode::OneMinusSrcColor},
            {BlendModeString::DstAlpha,         BlendMode::DstAlpha},
            {BlendModeString::OneMinusDstAlpha, BlendMode::OneMinusDstAlpha},
            {BlendModeString::ScrAlphaSaturate, BlendMode::ScrAlphaSaturate},
            {BlendModeString::OneMinusSrcAlpha, BlendMode::OneMinusSrcAlpha},
        };

        // NOTE(v.matushkin): I'm sure this is not gonna break
        return stringToBlendMode.find(str)->second;
    }

    template<>
    CullMode FromString<CullMode>(const std::string& str)
    {
        if (str == CullModeString::Off)   return CullMode::Off;
        if (str == CullModeString::Front) return CullMode::Front;

        // NOTE(v.matushkin): I'm sure this is not gonna break
        return CullMode::Back;
    }

    template<>
    DepthCompareFunction FromString<DepthCompareFunction>(const std::string& str)
    {
        static const std::unordered_map<std::string, DepthCompareFunction> stringToDepthCompareFunction
        {
            {DepthCompareFunctionString::Never,        DepthCompareFunction::Never},
            {DepthCompareFunctionString::Less,         DepthCompareFunction::Less},
            {DepthCompareFunctionString::Equal,        DepthCompareFunction::Equal},
            {DepthCompareFunctionString::LessEqual,    DepthCompareFunction::LessEqual},
            {DepthCompareFunctionString::Greater,      DepthCompareFunction::Greater},
            {DepthCompareFunctionString::NotEqual,     DepthCompareFunction::NotEqual},
            {DepthCompareFunctionString::GreaterEqual, DepthCompareFunction::GreaterEqual},
            {DepthCompareFunctionString::Always,       DepthCompareFunction::Always},
        };

        // NOTE(v.matushkin): I'm sure this is not gonna break
        return stringToDepthCompareFunction.find(str)->second;
    }

} // namespace snv::EnumUtils
