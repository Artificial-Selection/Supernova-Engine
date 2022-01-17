#pragma once

#include <Engine/Core/Core.hpp>

#include <optional>
#include <string>
#include <vector>


namespace snv
{

constexpr ui32 k_InvalidHandle = -1;

enum class BufferHandle        : ui32 { InvalidHandle = k_InvalidHandle };
enum class RenderPassHandle    : ui32 { InvalidHandle = k_InvalidHandle };
enum class RenderTextureHandle : ui32 { InvalidHandle = k_InvalidHandle };
enum class TextureHandle       : ui32 { InvalidHandle = k_InvalidHandle };
enum class ShaderHandle        : ui32 { InvalidHandle = k_InvalidHandle };


//- Shader states

//-- RasterizerState
enum class PolygonMode : ui8
{
    Fill,
    Wireframe,
};

enum class CullMode : ui8
{
    Off,
    Front,
    Back,
    // OpenGl and Vulkan have FrontAndBack
};

enum class TriangleFrontFace : ui8
{
    Clockwise,
    CounterClockwise,
};

//-- DepthStencilState
enum class DepthTest : ui8
{
    Off,
    ReadOnly,
    ReadWrite,
};

enum class CompareFunction : ui8
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

//-- BlendState
enum class BlendMode : ui8
{
    Off,
    BlendOp,
    LogicOp,
};

enum class BlendOp : ui8
{
    Add,
    Subtract,
    ReverseSubtract,
    Min,
    Max,
};

// NOTE(v.matushkin): OpenGL/Vulkan have more options
enum class BlendFactor : ui8
{
    Zero,
    One,
    SrcColor,
    OneMinusSrcColor,
    DstColor,
    OneMinusDstColor,
    SrcAlpha,
    OneMinusSrcAlpha,
    DstAlpha,
    OneMinusDstAlpha,
    ScrAlphaSaturate,
    // TODO(v.matushkin): How this '*1*' values work?
    Src1Color,
    OneMinusSrc1Color,
    Src1Alpha,
    OneMinusSrc1Alpha,
};

enum class BlendLogicOp : ui8
{
    Clear,
    Set,
    Copy,
    CopyInversed,
    Noop,
    Invert,
    And,
    Nand,
    Or,
    Nor,
    Xor,
    Equivalent,
    AndReverse,
    AndInverted,
    OrReverse,
    OrInverted,
};


// TODO(v.matushkin): Bring back enum class?
// Or use something from this https://walbourn.github.io/modern-c++-bitmask-types/
enum BufferBit : ui32
{
    Color   = 1 << 0,
    Depth   = 1 << 1,
    // Accum = 0x00000200,
    Stencil = 1 << 2,
};

enum class VertexAttribute : ui8
{
    Position,
    Normal,
    TexCoord0,
};

enum class VertexAttributeFormat : ui8
{
    SInt8,
    SInt16,
    SInt32,
    UInt8,
    UInt16,
    UInt32,
    SNorm8,
    SNorm16,
    UNorm8,
    UNorm16,
    Float16,
    Float32,
};

enum class RenderTextureLoadAction : ui8
{
    Clear,
    DontCare,
    Load,
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
    Depth32,
};

// NOTE(v.matushkin): Not sure about this enum
enum class RenderTextureType : ui8
{
    Color,
    Depth,
    Stencil,
    DepthStencil,
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
    DEPTH32F,
};
// TODO(v.matushkin): Make sure this works correctly in GPU API's
enum class TextureWrapMode : ui8
{
    ClampToEdge,
    ClampToBorder,
    MirroredOnce,
    MirroredRepeat,
    Repeat,
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
    std::string             Name; // NOTE(v.matushkin): Make std::string_view ?
    RenderTextureClearValue ClearValue;
    ui32                    Width;
    ui32                    Height;
    RenderTextureFormat     Format;
    RenderTextureLoadAction LoadAction;
    RenderTextureUsage      Usage;

    [[nodiscard]] RenderTextureType RenderTextureType() const;
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

struct RenderPassDesc
{
    std::vector<RenderTextureHandle>   ColorAttachments;
    std::optional<RenderTextureHandle> DepthStencilAttachment;
};

struct RasterizerStateDesc
{
    PolygonMode       PolygonMode;
    CullMode          CullMode;
    TriangleFrontFace FrontFace;

    static RasterizerStateDesc Default();
};

struct DepthStencilStateDesc
{
    bool            DepthTestEnable;
    bool            DepthWriteEnable;
    CompareFunction DepthCompareFunction;

    static DepthStencilStateDesc Default();
};

struct BlendStateDesc
{
    BlendMode    BlendMode;
    BlendFactor  ColorSrcBlendFactor;
    BlendFactor  ColorDstBlendFactor;
    BlendOp      ColorBlendOp;
    BlendFactor  AlphaSrcBlendFactor;
    BlendFactor  AlphaDstBlendFactor;
    BlendOp      AlphaBlendOp;
    BlendLogicOp LogicOp;

    static BlendStateDesc Default();
};

struct ShaderDesc
{
    std::string Name;

    RasterizerStateDesc   RasterizerStateDesc;
    DepthStencilStateDesc DepthStencilStateDesc;
    BlendStateDesc        BlendStateDesc;

    std::string VertexSource;
    std::string FragmentSource;
};

} // namespace snv
