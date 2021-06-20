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

#SET(MAIN_SRC
#        ${MAIN_PROJECT_FOLDER}/main.cpp
#        )
#SET(HEADERS
#        ${MAIN_PROJECT_FOLDER}/EventAction.h
#        ${MAIN_PROJECT_FOLDER}/Library/ObservableField.h
#        )
#SET(SOURCES
#        ${MAIN_PROJECT_FOLDER}/EventAction.cpp
#        ${MAIN_PROJECT_FOLDER}/Library/ObservableField.cpp
#        )

#add_library(lib ${SOURCES} ${HEADERS})
include_directories(${MAIN_PROJECT_FOLDER}/Library)

add_executable(EventSystem
        ${MAIN_PROJECT_FOLDER}/EventAction.cpp
        ${MAIN_PROJECT_FOLDER}/EventAction.h
        ${MAIN_PROJECT_FOLDER}/main.cpp
        source/EngineDefinitions.h source/Library/TestObservableField.cpp source/Library/TestObservableField.h)


#TARGET_LINK_LIBRARIES(EventSystem ObservableField)