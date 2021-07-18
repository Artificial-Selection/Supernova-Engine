#pragma once

#include <Core/Log.hpp>

#include <cstdlib>


// TODO(v.matushkin): Define platform macro
#ifdef SNV_PLATFORM_WINPC
    #define SNV_DEBUG_BREAK() __debugbreak()
#else
    #define SNV_DEBUG_BREAK()
#endif


#ifdef SNV_ASSERTS_ENABLED
    #define SNV_ASSERT(condition, message)\
        do\
        {\
            if ((condition) == false)\
            {\
                LOG_CRITICAL(\
                    "Assertion `" #condition "` failed\n"\
                    "\tFile: {}\n"\
                    "\tLine: {}\n"\
                    "\tMessage: {}",\
                    __FILE__, __LINE__, (message)\
                );\
                SNV_DEBUG_BREAK();\
                abort();\
            }\
        }\
        while ((void)0,0)
#else
    #define SNV_ASSERT(condition, message) do { } while ((void)0,0)
#endif // SNV_ASSERTS_ENABLED
