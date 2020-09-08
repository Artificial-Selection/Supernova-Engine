cmake_minimum_required(VERSION 3.15)
project(EventSystem)

set(CMAKE_CXX_STANDARD 20)
file(GLOB EventSystem_SRC
        "source/*.h"
        "source/*.cpp"
        )
add_executable(EventSystem ${EventSystem_SRC})