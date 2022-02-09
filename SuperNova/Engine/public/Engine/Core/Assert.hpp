#pragma once

#include <Engine/Core/Log.hpp>

#include <cstdlib>


// TODO(v.matushkin): https://artificial-mind.net/blog/2020/10/17/static-registration-macro


// TODO(v.matushkin): This should depend on compiler, not on platform, but there is no macro for it right now
#ifdef SNV_PLATFORM_WINDOWS
    #define SNV_DEBUG_BREAK() __debugbreak()
#else
    #define SNV_DEBUG_BREAK()
#endif


#ifdef SNV_ASSERTS_ENABLED
    #define SNV_ASSERT(condition, message)                \
        do                                                \
        {                                                 \
            if ((condition) == false)                     \
            {                                             \
                LOG_CRITICAL(                             \
                    "Assertion `" #condition "` failed\n" \
                    "\tFile: {}\n"                        \
                    "\tLine: {}\n"                        \
                    "\tMessage: {}",                      \
                    __FILE__, __LINE__, (message)         \
                );                                        \
                SNV_DEBUG_BREAK();                        \
                abort();                                  \
            }                                             \
        }                                                 \
        while ((void)0,0)
#else
    #define SNV_ASSERT(condition, message) do { } while ((void)0,0)
#endif // SNV_ASSERTS_ENABLED
