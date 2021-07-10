#pragma once

#include <cstdint>
#include <iostream>

using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using ui8 = std::uint8_t;
using ui16 = std::uint16_t;
using ui32 = std::uint32_t;
using ui64 = std::uint64_t;

using f32 = float;
using f64 = double;


#define SNV_ENABLE_DEBUG

#ifdef SNV_ENABLE_DEBUG

#define SNV_ENABLE_LOGGING

#define HALT(condition, message) \
do { \
if (! (condition)) { \
std::cerr << "Assertion `" #condition "` failed in " << __FILE__ \
<< " line " << __LINE__ << ": " << (message) << std::endl; \
std::terminate(); \
} \
} while ((void)0,0)
#else
#define HALT(condition, message) do { } while ((void)0,0)

#endif // SNV_ENABLE_DEBUG
