#pragma once

#include <Core/Core.hpp>


namespace snv
{

enum class GraphicsApi : ui8
{
    OpenGL,
#ifdef SNV_PLATFORM_WINDOWS
    DirectX11,
    DirectX12
#endif
};


enum class BlendFactor : ui32
{
    One,
    SrcAlpha,
    OneMinusSrcAlpha
};

enum class DepthFunction : ui32
{
    Never,
    Less,
    Equal,
    LessOrEqual,
    Greater,
    NotEqual,
    GreaterOrEqual,
    Always
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

enum class BufferHandle  : ui32 { InvalidHandle = k_InvalidHandle };
enum class TextureHandle : ui32 { InvalidHandle = k_InvalidHandle };
enum class ShaderHandle  : ui32 { InvalidHandle = k_InvalidHandle };


enum class VertexAttribute : ui8
{
    Position,
    Normal,
    TexCoord0
};

enum class VertexAttributeFormat : ui8
{
    Int8,
    Int16,
    Int32,
    UInt8,
    UInt16,
    UInt32,
    Float16,
    Float32,
    Float64
};

struct VertexAttributeDesc
{
    VertexAttribute       Attribute;
    VertexAttributeFormat Format;
    ui8                   Dimension;
    ui32                  Offset;
};

enum class TextureFormat : ui8
{
    R8,
    R16,
    R16F,
    R32F,
    RG8,
    RG16,
    // RGB8,
    // RGB16,
    RGBA8,
    RGBA16F,
    DEPTH16,
    // DEPTH24,
    DEPTH32, // TODO(v.matushkin): Should be called D24S8 or smth like that?
    DEPTH32F
};
// TODO(v.matushkin): Make sure this works correctly in GPU API's
enum class TextureWrapMode : ui8
{
    ClampToEdge,
    ClampToBorder,
    MirroredOnce,
    MirroredRepeat,
    Repeat
};

struct TextureDesc
{
    ui32            Width;
    ui32            Height;
    TextureFormat   Format;
    TextureWrapMode WrapMode;
};

} // namespace snv
