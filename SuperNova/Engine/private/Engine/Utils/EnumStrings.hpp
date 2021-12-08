#pragma once


// TODO(v.matushkin): const char* -> std::string_view ?


namespace snv
{

// Engine/Assets/ShaderParser/Lexer.hpp

namespace TokenString
{
    //- Identifiers
    inline const char* Identifier                 = "Identifier";
    inline const char* Shader                     = "Shader";
    //-- State
    inline const char* State                      = "State";
    inline const char* Blend                      = "Blend";
    inline const char* Cull                       = "Cull";
    inline const char* DepthTest                  = "DepthTest";
    inline const char* DepthCompareFunction       = "DepthCompareFunction";
    //-- Shader Stage
    inline const char* Vertex                     = "Vertex";
    inline const char* Fragment                   = "Fragment";

    //- Values
    inline const char* Value_Number               = "Value_Number";
    inline const char* Value_String               = "Value_String";
    //-- State Value
    inline const char* Value_On                   = "Value_On";
    inline const char* Value_Off                  = "Value_Off";
    inline const char* Value_BlendMode            = "Value_BlendMode";
    inline const char* Value_CullMode             = "Value_CullMode";
    inline const char* Value_DepthCompareFunction = "Value_DepthCompareFunction";

    //- Common
    inline const char* OpenBrace                  = "{";
    inline const char* CloseBrace                 = "}";
    inline const char* Comma                      = ",";

    inline const char* EndOfFile                  = "EndOfFile";
    inline const char* Undefined                  = "Undefined";
}


// Engine/Renderer/RenderTypes.hpp

namespace BlendModeString
{
    inline const char* Zero             = "Zero";
    inline const char* One              = "One";
    inline const char* DstColor         = "DstColor";
    inline const char* SrcColor         = "SrcColor";
    inline const char* OneMinusDstColor = "OneMinusDstColor";
    inline const char* SrcAlpha         = "SrcAlpha";
    inline const char* OneMinusSrcColor = "OneMinusSrcColor";
    inline const char* DstAlpha         = "DstAlpha";
    inline const char* OneMinusDstAlpha = "OneMinusDstAlpha";
    inline const char* ScrAlphaSaturate = "ScrAlphaSaturate";
    inline const char* OneMinusSrcAlpha = "OneMinusSrcAlpha";
}

namespace CullModeString
{
    inline const char* Off   = "Off";
    inline const char* Front = "Front";
    inline const char* Back  = "Back";
}

namespace DepthCompareFunctionString
{
    inline const char* Never        = "Never";
    inline const char* Less         = "Less";
    inline const char* Equal        = "Equal";
    inline const char* LessEqual    = "LessEqual";
    inline const char* Greater      = "Greater";
    inline const char* NotEqual     = "NotEqual";
    inline const char* GreaterEqual = "GreaterEqual";
    inline const char* Always       = "Always";
}

} // namespace snv
