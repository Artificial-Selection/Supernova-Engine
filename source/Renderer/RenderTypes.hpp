#pragma once

#include <Core/Core.hpp>


namespace snv
{

enum class GraphicsApi : ui8
{
    OpenGL,
#ifdef SNV_PLATFORM_WINDOWS
    DirectX11
#endif
};


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

enum class GraphicsBufferHandle : ui32 { InvalidHandle = k_InvalidHandle };
enum class TextureHandle        : ui32 { InvalidHandle = k_InvalidHandle };
enum class ShaderHandle         : ui32 { InvalidHandle = k_InvalidHandle };


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

// TODO(v.matushkin): Rename to TextureFormat? wtf did I even called it like that
enum class TextureGraphicsFormat : ui8
{
    R8,
    R16,
    R16F,
    R32F,
    RG8,
    RG16,
    RGB8,
    RGB16,
    RGBA8,
    RGBA16F,
    DEPTH16,
    DEPTH24,
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

struct TextureDescriptor
{
    i32                   Width;
    i32                   Height;
    TextureGraphicsFormat GraphicsFormat; // TODO(v.matushkin): Rename to just Format?
    TextureWrapMode       WrapMode;
};

} // namespace snv
