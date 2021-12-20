#pragma once

#include <Engine/Core/Core.hpp>

#include <vector>
#include <string>


namespace snv
{

constexpr ui32 k_InvalidHandle = -1;

enum class BufferHandle        : ui32 { InvalidHandle = k_InvalidHandle };
enum class FramebufferHandle   : ui32 { InvalidHandle = k_InvalidHandle };
enum class RenderTextureHandle : ui32 { InvalidHandle = k_InvalidHandle };
enum class TextureHandle       : ui32 { InvalidHandle = k_InvalidHandle };
enum class ShaderHandle        : ui32 { InvalidHandle = k_InvalidHandle };


// NOTE(v.matushkin): Unity has 'Off' BlendMode, what does it do?
// NOTE(v.matushkin): Graphics APIs have more options
enum class BlendMode : ui8
{
    Zero,
    One,
    DstColor,
    SrcColor,
    OneMinusDstColor,
    SrcAlpha,
    OneMinusSrcColor,
    DstAlpha,
    OneMinusDstAlpha,
    ScrAlphaSaturate,
    OneMinusSrcAlpha,
};

// TODO(v.matushkin): Bring back enum class?
// Or use something from this https://walbourn.github.io/modern-c++-bitmask-types/
enum BufferBit : ui32
{
    Color   = 1 << 0,
    Depth   = 1 << 1,
    // Accum = 0x00000200,
    Stencil = 1 << 2
};

enum class CullMode : ui8
{
    Off,
    Front,
    Back,
    // OpenGl and Vulkan have FrontAndBack
};

enum class DepthCompareFunction : ui8
{
    Never,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always,
};

// NOTE(v.matushkin): Not sure about this enum
enum class FramebufferDepthStencilType : ui8
{
    None,
    Depth,
    Stencil,
    DepthStencil
};

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

enum class RenderTextureLoadAction : ui8
{
    Clear,
    DontCare,
    Load
};

// NOTE(v.matushkin): Not sure about this enum and the naming
enum class RenderTextureUsage : ui8
{
    Default,
    ShaderRead,
};

// TODO(v.matushkin): Add Default color/depth formats that depends on the current platform
enum class RenderTextureFormat : ui8
{
    BGRA32,
    Depth32
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


// NOTE(v.matushkin): Default Color/DepthStencil clear values?
struct ClearDepthStencilValue
{
    f32 Depth;
    ui8 Stencil;
};

union RenderTextureClearValue
{
    f32                    Color[4];
    ClearDepthStencilValue DepthStencil;
};

struct RenderTextureDesc
{
    RenderTextureClearValue ClearValue;
    ui32                    Width;
    ui32                    Height;
    RenderTextureFormat     Format;
    RenderTextureLoadAction LoadAction;
    RenderTextureUsage      Usage;
};

struct TextureDesc
{
    ui32            Width;
    ui32            Height;
    TextureFormat   Format;
    TextureWrapMode WrapMode;
};

struct VertexAttributeDesc
{
    VertexAttribute       Attribute;
    VertexAttributeFormat Format;
    ui8                   Dimension;
    ui32                  Offset;
};

struct GraphicsStateDesc
{
    std::vector<RenderTextureDesc> ColorAttachments;
    RenderTextureDesc              DepthStencilAttachment;
    FramebufferDepthStencilType    DepthStencilType;
};

struct GraphicsState
{
    std::vector<RenderTextureHandle> ColorAttachments;
    RenderTextureHandle              DepthStencilAttachment;
    FramebufferHandle                Framebuffer;
};

struct BlendState
{
    BlendMode ColorSrcBlendMode;
    BlendMode ColorDstBlendMode;
    BlendMode AlphaSrcBlendMode;
    BlendMode AlphaDstBlendMode;
};

struct ShaderState
{
    BlendState           BlendState;
    CullMode             CullMode;
    DepthCompareFunction DepthCompareFunction;

    static ShaderState Default();
};

struct ShaderDesc
{
    std::string Name;

    ShaderState State;

    std::string VertexSource;
    std::string FragmentSource;
};

} // namespace snv
