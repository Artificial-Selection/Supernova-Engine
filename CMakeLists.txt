cmake_minimum_required(VERSION 3.15)
project(EventSystem)

set(CMAKE_CXX_STANDARD 17)
if(MSVC)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
    set(BUILD_SHARED_LIBS TRUE)
endif()

set(CMAKE_VERBOSE_MAKEFILE ON)

SET(MAIN_PROJECT_FOLDER
        source/)

include_directories(${MAIN_PROJECT_FOLDER}/Library)

add_executable(EventSystem
        ${MAIN_PROJECT_FOLDER}/main.cpp
        source/EngineDefinitions.h)


#TARGET_LINK_LIBRARIES(EventSystem ObservableField)