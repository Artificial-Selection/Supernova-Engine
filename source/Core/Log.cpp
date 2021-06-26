#include <Core/Log.hpp>

//#include <spdlog/sinks/stdout_color_sinks.h>
//#include <spdlog/sinks/basic_file_sink.h>
//
//#include <vector>


//std::shared_ptr<spdlog::logger> Log::s_logger;


//void Log::Init(spdlog::level::level_enum logLevel)
//{
//    std::vector<spdlog::sink_ptr> logSinks;
//    logSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
//    //logSinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("Log.txt", true));
//
//    logSinks[0]->set_pattern("%^[%T] %n: %v%$");
//    //logSinks[1]->set_pattern("[%T] [%l] %n: %v");
//
//    s_logger = std::make_shared<spdlog::logger>("LOGGER", std::begin(logSinks), std::end(logSinks));
//    spdlog::register_logger(s_logger);
//    s_logger->set_level( logLevel );
//    //s_logger->flush_on(spdlog::level::trace);
//}
