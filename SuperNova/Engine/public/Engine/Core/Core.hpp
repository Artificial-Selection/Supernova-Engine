#pragma once

#include <cstdint>


using i8  = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using ui8  = std::uint8_t;
using ui16 = std::uint16_t;
using ui32 = std::uint32_t;
using ui64 = std::uint64_t;

using f32 = float;
using f64 = double;


// TODO(v.matushkin): CMake should define this
#define SNV_CONFIGURATION_DEBUG


// TODO(v.matushkin): Replace all '#define *_ENABLED' with '#define *_ENABLED [true|false]'
//  and all '#ifdef *_ENABLED' with '#if *_ENABLED'

#if defined(SNV_CONFIGURATION_DEBUG)

    #define SNV_LOGGING_ENABLED
    #define SNV_ASSERTS_ENABLED
    #define SNV_GPU_API_DEBUG_ENABLED
    #define SNV_GPU_API_DEBUG_NAMES_ENABLED true

#elif defined(SNV_CONFIGURATION_RELEASE)

    #define SNV_LOGGING_ENABLED
    #define SNV_ASSERTS_ENABLED
    #define SNV_GPU_API_DEBUG_ENABLED
    #define SNV_GPU_API_DEBUG_NAMES_ENABLED true

#elif defined(SNV_CONFIGURATION_FINAL)

    #define SNV_GPU_API_DEBUG_NAMES_ENABLED false

#else
    #error Undefined build configuration
#endif 


// TODO(v.matushkin): Compiler dependent macros
//  Use new attribute syntax? Eg. [[gnu::pure]]
//  https://artificial-mind.net/blog/2021/10/17/optimize-without-inline

// TODO(v.matushkin): This should depend on compiler, not on platform, but there is no macro for compiler right now
#ifdef SNV_PLATFORM_WINDOWS

    #define SNV_FUNC_INLINE       inline
    #define SNV_FUNC_FORCE_INLINE __forceinline

#else
    #error Unsupported platform
#endif
