cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
project(SuperNova CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


if(MSVC)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
    set(BUILD_SHARED_LIBS TRUE)
endif()

# ------------------------- Conan setup -------------------------
set(BUILD_DIR ${PROJECT_SOURCE_DIR}/build)
set(CONAN_DIR ${BUILD_DIR}/conan)
set(CONAN_BUILD_INFO_FILE ${CONAN_DIR}/conanbuildinfo.txt)

if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
    message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
    file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/v0.16.1/conan.cmake"
                  "${CONAN_DIR}/conan.cmake"
                  EXPECTED_HASH SHA256=396e16d0f5eabdc6a14afddbcfff62a54a7ee75c6da23f32f7a31bc85db23484
                  TLS_VERIFY ON)
endif()

include(${CONAN_DIR}/conan.cmake)
conan_cmake_autodetect(settings)
conan_cmake_run(
    CONANFILE ${PROJECT_SOURCE_DIR}/conanfile.py
    CMAKE_TARGETS
    INSTALL_FOLDER ${CONAN_DIR}
    BUILD missing
)

if(EXISTS ${CONAN_BUILD_INFO_FILE})
    set(CMAKE_MODULE_PATH ${CONAN_DIR} ${CMAKE_MODULE_PATH})
    set(CMAKE_PREFIX_PATH ${CONAN_DIR} ${CMAKE_PREFIX_PATH})
else()
    message(ERROR "The file ${CONAN_BUILD_INFO_FILE} doesn't exist, you're fucked, comrade")
endif()


find_package(glad   REQUIRED)
find_package(glfw3  REQUIRED)
find_package(glm    REQUIRED)
find_package(EnTT   REQUIRED)
find_package(spdlog REQUIRED)


set(MAIN_SRC_DIR ${PROJECT_SOURCE_DIR}/source)

# ---------------------------- Core -----------------------------
set(Core_SRC_DIR ${MAIN_SRC_DIR}/Core)
set(Core_SRC
    ${Core_SRC_DIR}/Core.hpp
    ${Core_SRC_DIR}/Log.hpp
    ${Core_SRC_DIR}/Log.cpp
)

# -------------------------- Renderer ---------------------------
set(Renderer_SRC_DIR ${MAIN_SRC_DIR}/Renderer)

set(OpenGL_SRC_DIR ${Renderer_SRC_DIR}/OpenGL)
set(OpenGL_SRC
    ${OpenGL_SRC_DIR}/GLBackend.hpp
    ${OpenGL_SRC_DIR}/GLBackend.cpp
    ${OpenGL_SRC_DIR}/GLShader.hpp
    ${OpenGL_SRC_DIR}/GLShader.cpp
)

set(Renderer_SRC
    ${OpenGL_SRC}
)


add_executable(SuperNova
    ${MAIN_SRC_DIR}/main.cpp
    ${MAIN_SRC_DIR}/EngineDefinitions.h
    ${Core_SRC}
    ${Renderer_SRC}
)
target_link_libraries(SuperNova PRIVATE
    glad::glad
    glfw::glfw
    glm::glm
    EnTT::EnTT
    spdlog::spdlog
)
target_include_directories(SuperNova PRIVATE
    ${MAIN_SRC_DIR}
)

#TARGET_LINK_LIBRARIES(EventSystem ObservableField)
