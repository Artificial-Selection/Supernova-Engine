#pragma once

#include <Core/Core.hpp>


namespace snv
{

enum class BlendFactor : ui32
{
    One              = 0,
    SrcAlpha         = 1,
    OneMinusSrcAlpha = 2
};

enum class DepthFunction : ui32
{
    Never          = 0,
    Less           = 1,
    Equal          = 2,
    LessOrEqual    = 3,
    Greater        = 4,
    NotEqual       = 5,
    GreaterOrEqual = 6,
    Always         = 7
};

// TODO(v.matushkin): Bring back enum class?
// Or use something from this https://walbourn.github.io/modern-c++-bitmask-types/
enum BufferBit : ui32
{
    Color   = 1 << 0,
    Depth   = 1 << 1,
    //Accum = 0x00000200,
    Stencil = 1 << 2
};



constexpr ui32 k_InvalidHandle = -1;

enum class GraphicsBufferHandle : ui32 { id = k_InvalidHandle };


enum class VertexAttribute
{
    Position  = 0,
    Normal    = 1,
    TexCoord0 = 2
};

enum class VertexAttributeFormat
{
    Int8    = 0,
    Int16   = 1,
    Int32   = 2,
    UInt8   = 3,
    UInt16  = 4,
    UInt32  = 5,
    Float16 = 6,
    Float32 = 7,
    Float64 = 8
};

struct VertexAttributeDescriptor
{
    VertexAttribute       Attribute;
    VertexAttributeFormat Format;
    ui32                  Dimension;
    ui32                  Offset;
};

} // namespace snv
