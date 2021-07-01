#pragma once

#include <Core/Core.hpp>


namespace snv
{

enum class GLBlendFactor
{
    One              = 1,
    SrcAlpha         = 0x0302,
    OneMinusSrcAlpha = 0x0303
};

enum class GLDepthFunction
{
    Never          = 0x0200,
    Less           = 0x0201,
    Equal          = 0x0202,
    LessOrEqual    = 0x0203,
    Greater        = 0x0204,
    NotEqual       = 0x0205,
    GreaterOrEqual = 0x0206,
    Always         = 0x0207
};

enum class GLBufferBit : ui32
{
    Color   = 1 << 14,
    Depth   = 1 << 8,
    //Accum   = 0x00000200,
    Stencil = 1 << 10
};

class GLBackend
{
public:
    static void Init();

    static void EnableBlend();
    static void EnableDepthTest();

    static void SetBlendFunction(GLBlendFactor source, GLBlendFactor destination);
    static void SetClearColor(f32 r, f32 g, f32 b, f32 a);
    static void SetDepthFunction(GLDepthFunction depthFunction);
    static void SetViewport(i32 x, i32 y, i32 width, i32 height);

    static void Clear(GLBufferBit bufferBitMask);

    static void DrawArrays(i32 count);
};

} // namespace snv
