#pragma once

#include <Engine/Core/Core.hpp>

#include <spdlog/spdlog.h>

#include <memory>


//class Log
//{
//public:
//    static void Init( spdlog::level::level_enum logLevel );

    //inline static std::shared_ptr<spdlog::logger>& GetLogger() { return s_logger; }

//private:
//    static std::shared_ptr<spdlog::logger> s_logger;
//};

#ifdef SNV_LOGGING_ENABLED
    /*#define LOG_TRACE(...)    ::Log::GetLogger()->trace(__VA_ARGS__)
    #define LOG_INFO(...)     ::Log::GetLogger()->info(__VA_ARGS__)
    #define LOG_WARN(...)     ::Log::GetLogger()->warn(__VA_ARGS__)
    #define LOG_ERROR(...)    ::Log::GetLogger()->error(__VA_ARGS__)
    #define LOG_CRITICAL(...) ::Log::GetLogger()->critical(__VA_ARGS__)*/

    #define LOG_TRACE(...)    SPDLOG_TRACE(__VA_ARGS__)
    #define LOG_INFO(...)     SPDLOG_INFO(__VA_ARGS__)
    #define LOG_WARN(...)     SPDLOG_WARN(__VA_ARGS__)
    #define LOG_ERROR(...)    SPDLOG_ERROR(__VA_ARGS__)
    #define LOG_CRITICAL(...) SPDLOG_CRITICAL(__VA_ARGS__)
#else
    #define LOG_TRACE(...)
    #define LOG_INFO(...)
    #define LOG_WARN(...)
    #define LOG_ERROR(...)
    #define LOG_CRITICAL(...)
#endif
