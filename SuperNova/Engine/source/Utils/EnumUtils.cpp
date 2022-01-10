#include <Engine/Utils/EnumUtils.hpp>

#include <Engine/Assets/ShaderParser/Lexer.hpp>
#include <Engine/Utils/EnumStrings.hpp>

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
            TokenString::Cull,
            TokenString::DepthWrite,
            TokenString::DepthTest,
            TokenString::DepthCompare,
            TokenString::BlendOp,
            TokenString::Blend,
            //-- Shader Stage
            TokenString::Vertex,
            TokenString::Fragment,

            //- Values
            TokenString::Value_Number,
            TokenString::Value_String,
            //-- State Value
            TokenString::Value_On,
            TokenString::Value_Off,
            TokenString::Value_CullMode,
            TokenString::Value_CompareFunction,
            TokenString::Value_BlendOp,
            TokenString::Value_BlendFactor,

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

    std::string ToString(CompareFunction compareFunction)
    {
        static const char* compareFunctionToString[]
        {
            CompareFunctionString::Never,
            CompareFunctionString::Less,
            CompareFunctionString::Equal,
            CompareFunctionString::LessEqual,
            CompareFunctionString::Greater,
            CompareFunctionString::NotEqual,
            CompareFunctionString::GreaterEqual,
            CompareFunctionString::Always,
        };

        return compareFunctionToString[static_cast<ui8>(compareFunction)];
    }

    std::string ToString(BlendOp blendOp)
    {
        static const char* blendOpToString[]
        {
            BlendOpString::Add,
            BlendOpString::Subtract,
            BlendOpString::ReverseSubtract,
            BlendOpString::Min,
            BlendOpString::Max,
        };

        return blendOpToString[static_cast<ui8>(blendOp)];
    }

    std::string ToString(BlendFactor blendFactor)
    {
        static const char* blendFactorToString[]
        {
            BlendFactorString::Zero,
            BlendFactorString::One,
            BlendFactorString::SrcColor,
            BlendFactorString::OneMinusSrcColor,
            BlendFactorString::DstColor,
            BlendFactorString::OneMinusDstColor,
            BlendFactorString::SrcAlpha,
            BlendFactorString::OneMinusSrcAlpha,
            BlendFactorString::DstAlpha,
            BlendFactorString::OneMinusDstAlpha,
            BlendFactorString::ScrAlphaSaturate,
            BlendFactorString::Src1Color,
            BlendFactorString::OneMinusSrc1Color,
            BlendFactorString::Src1Alpha,
            BlendFactorString::OneMinusSrc1Alpha,
        };

        return blendFactorToString[static_cast<ui8>(blendFactor)];
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
    CompareFunction FromString<CompareFunction>(const std::string& str)
    {
        static const std::unordered_map<std::string, CompareFunction> stringToCompareFunction
        {
            {CompareFunctionString::Never,        CompareFunction::Never},
            {CompareFunctionString::Less,         CompareFunction::Less},
            {CompareFunctionString::Equal,        CompareFunction::Equal},
            {CompareFunctionString::LessEqual,    CompareFunction::LessEqual},
            {CompareFunctionString::Greater,      CompareFunction::Greater},
            {CompareFunctionString::NotEqual,     CompareFunction::NotEqual},
            {CompareFunctionString::GreaterEqual, CompareFunction::GreaterEqual},
            {CompareFunctionString::Always,       CompareFunction::Always},
        };

        // NOTE(v.matushkin): I'm sure this is not gonna break
        return stringToCompareFunction.find(str)->second;
    }

    template<>
    BlendOp FromString<BlendOp>(const std::string& str)
    {
        static const std::unordered_map<std::string, BlendOp> stringToBlendOp
        {
            {BlendOpString::Add,             BlendOp::Add},
            {BlendOpString::Subtract,        BlendOp::Subtract},
            {BlendOpString::ReverseSubtract, BlendOp::ReverseSubtract},
            {BlendOpString::Min,             BlendOp::Min},
            {BlendOpString::Max,             BlendOp::Max},
        };

        // NOTE(v.matushkin): I'm sure this is not gonna break
        return stringToBlendOp.find(str)->second;
    }

    template<>
    BlendFactor FromString<BlendFactor>(const std::string& str)
    {
        static const std::unordered_map<std::string, BlendFactor> stringToBlendFactor
        {
            {BlendFactorString::Zero,              BlendFactor::Zero},
            {BlendFactorString::One,               BlendFactor::One},
            {BlendFactorString::SrcColor,          BlendFactor::SrcColor},
            {BlendFactorString::OneMinusSrcColor,  BlendFactor::OneMinusSrcColor},
            {BlendFactorString::DstColor,          BlendFactor::DstColor},
            {BlendFactorString::OneMinusDstColor,  BlendFactor::OneMinusDstColor},
            {BlendFactorString::SrcAlpha,          BlendFactor::SrcAlpha},
            {BlendFactorString::OneMinusSrcAlpha,  BlendFactor::OneMinusSrcAlpha},
            {BlendFactorString::DstAlpha,          BlendFactor::DstAlpha},
            {BlendFactorString::OneMinusDstAlpha,  BlendFactor::OneMinusDstAlpha},
            {BlendFactorString::ScrAlphaSaturate,  BlendFactor::ScrAlphaSaturate},
            {BlendFactorString::Src1Color,         BlendFactor::Src1Color},
            {BlendFactorString::OneMinusSrc1Color, BlendFactor::OneMinusSrc1Color},
            {BlendFactorString::Src1Alpha,         BlendFactor::Src1Alpha},
            {BlendFactorString::OneMinusSrc1Alpha, BlendFactor::OneMinusSrc1Alpha},
        };

        // NOTE(v.matushkin): I'm sure this is not gonna break
        return stringToBlendFactor.find(str)->second;
    }

} // namespace snv::EnumUtils
