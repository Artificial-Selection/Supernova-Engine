#pragma once

#include <Engine/Core/Core.hpp>


namespace snv::Input
{

enum class KeyboardKey : i32
{
    // Printable keys

    Space          = 32,

    Quote          = 39, // '

    Comma          = 44, // ,
    Minus          = 45, // -
    Period         = 46, // .
    Slash          = 47, // /
    Digit0         = 48,
    Digit1         = 49,
    Digit2         = 50,
    Digit3         = 51,
    Digit4         = 52,
    Digit5         = 53,
    Digit6         = 54,
    Digit7         = 55,
    Digit8         = 56,
    Digit9         = 57,

    Semicolon      = 59, // ;
    Equals         = 61, // =

    A              = 65,
    B              = 66,
    C              = 67,
    D              = 68,
    E              = 69,
    F              = 70,
    G              = 71,
    H              = 72,
    I              = 73,
    J              = 74,
    K              = 75,
    L              = 76,
    M              = 77,
    N              = 78,
    O              = 79,
    P              = 80,
    Q              = 81,
    R              = 82,
    S              = 83,
    T              = 84,
    U              = 85,
    V              = 86,
    W              = 87,
    X              = 88,
    Y              = 89,
    Z              = 90,
    LeftBracket    = 91, // [
    Backslash      = 92, // '\'
    RightBracket   = 93, // ]
    BackQuote      = 96, // `

    // Functional Keys

    Escape         = 256,
    Enter          = 257,
    Tab            = 258,
    Backspace      = 259,
    Insert         = 260,
    Delete         = 261,
    RightArrow     = 262,
    LeftArrow      = 263,
    DownArrow      = 264,
    UpArrow        = 265,
    PageUp         = 266,
    PageDown       = 267,
    Home           = 268,
    End            = 269,

    CapsLock       = 280,
    ScrollLock     = 281,
    NumLock        = 282,
    PrintScreen    = 283,
    Pause          = 284,

    F1             = 290,
    F2             = 291,
    F3             = 292,
    F4             = 293,
    F5             = 294,
    F6             = 295,
    F7             = 296,
    F8             = 297,
    F9             = 298,
    F10            = 299,
    F11            = 300,
    F12            = 301,
    // F13         = 302,  // NOTE(v.matushkin): The fuck is this?
    // F14         = 303,
    // F15         = 304,
    // F16         = 305,
    // F17         = 306,
    // F18         = 307,
    // F19         = 308,
    // F20         = 309,
    // F21         = 310,
    // F22         = 311,
    // F23         = 312,
    // F24         = 313,
    // F25         = 314,

    Numpad0        = 320,
    Numpad1        = 321,
    Numpad2        = 322,
    Numpad3        = 323,
    Numpad4        = 324,
    Numpad5        = 325,
    Numpad6        = 326,
    Numpad7        = 327,
    Numpad8        = 328,
    Numpad9        = 329,
    NumpadPeriod   = 330,
    NumpadDivide   = 331,
    NumpadMultiply = 332,
    NumpadMinus    = 333,
    NumpadPlus     = 334,
    NumpadEnter    = 335,
    // GLFW_KEY_KP_EQUAL = 336,  // NOTE(v.matushkin): The fuck is this?

    LeftShift      = 340,
    LeftControl    = 341,
    LeftAlt        = 342,
    LeftMeta       = 343, // Windows|Apple|Command Key
    RightShift     = 344,
    RightControl   = 345,
    RightAlt       = 346,
    RightMeta      = 347, // Windows|Apple|Command Key
    Menu           = 348
};

} // namespace snv::Input
