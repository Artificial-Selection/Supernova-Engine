cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
project(SuperNova CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(SNV_PLATFORM_WINDOWS 1)
    add_compile_definitions(
        SNV_PLATFORM_WINDOWS
        WIN32_LEAN_AND_MEAN     # To strip windows.h include. May be there is more macros?
    )
# elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
#     add_compile_definitions(SNV_PLATFORM_MAC)
# elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
#     add_compile_definitions(SNV_PLATFORM_LINUX)
else()
    message(FATAL_ERROR "Unsupported OS")
endif()


if(MSVC)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
    set(BUILD_SHARED_LIBS TRUE)
endif()


set(BUILD_DIR     ${PROJECT_SOURCE_DIR}/build)
set(LIBS_DIR      ${PROJECT_SOURCE_DIR}/libs)
set(SuperNova_DIR ${PROJECT_SOURCE_DIR}/SuperNova)
set(CONAN_DIR     ${BUILD_DIR}/conan)


# ----------------------------------------------------------------------------------------------------
# ------------------------------------------- Conan setup --------------------------------------------
# ----------------------------------------------------------------------------------------------------
set(ENV{CONAN_USER_HOME} ${PROJECT_SOURCE_DIR})

set(CONAN_BUILD_INFO_FILE ${CONAN_DIR}/conanbuildinfo.txt)

if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
    message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
    file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/v0.16.1/conan.cmake"
                  "${CONAN_DIR}/conan.cmake"
                  EXPECTED_HASH SHA256=396e16d0f5eabdc6a14afddbcfff62a54a7ee75c6da23f32f7a31bc85db23484
                  TLS_VERIFY ON)
endif()

include(${CONAN_DIR}/conan.cmake)
# NOTE(v.matushkin): CMakePresets.json:
#  conan-cmake cries about CMAKE_BUILD_TYPE being not set when using visual studio generator is there a way to fix it?
# NOTE(v.matushkin): How does this work for single configuration types?
foreach(TYPE ${CMAKE_CONFIGURATION_TYPES})
    conan_cmake_autodetect(settings BUILD_TYPE ${TYPE})
conan_cmake_run(
    CONANFILE ${PROJECT_SOURCE_DIR}/conanfile.py
    CMAKE_TARGETS
    INSTALL_FOLDER ${CONAN_DIR}
    BUILD missing
)
endforeach()


if(EXISTS ${CONAN_BUILD_INFO_FILE})
    set(CMAKE_MODULE_PATH ${CONAN_DIR} ${CMAKE_MODULE_PATH})
    set(CMAKE_PREFIX_PATH ${CONAN_DIR} ${CMAKE_PREFIX_PATH})
else()
    message(FATAL_ERROR "The file ${CONAN_BUILD_INFO_FILE} doesn't exist, you're fucked, comrade")
endif()


# ----------------------------------------------------------------------------------------------------
# ------------------------------------------ 3rdparty libs -------------------------------------------
# ----------------------------------------------------------------------------------------------------
set(LIBS_INCLUDE_DIR ${LIBS_DIR}/include)
set(LIBS_STATIC_DIR  ${LIBS_DIR}/lib)
set(LIBS_SHARED_DIR  ${LIBS_DIR}/shared)

# --------------------------- DXC ----------------------------
set(DXC_STATIC
    ${LIBS_STATIC_DIR}/dxcompiler.lib
)
set(DXC_SHARED
    ${LIBS_SHARED_DIR}/dxcompiler.dll
    ${LIBS_SHARED_DIR}/dxil.dll
)

# -------------------------- Conan ---------------------------
find_package(assimp REQUIRED)
find_package(EnTT   REQUIRED)
find_package(glad   REQUIRED)
find_package(glfw3  REQUIRED)
find_package(glm    REQUIRED)
find_package(imgui  REQUIRED)
find_package(spdlog REQUIRED)
find_package(stb    REQUIRED)


# --------------------- SuperNova-Engine ---------------------
set(SuperNovaEngine_LIBS_PUBLIC
    EnTT::EnTT
)
set(SuperNovaEngine_LIBS_PRIVATE
    assimp::assimp
    glad::glad
    glfw::glfw
    glm::glm
    spdlog::spdlog
    stb::stb
)

if(SNV_PLATFORM_WINDOWS)
    # NOTE: d3dcompiler/dxgi should be in DirectXCommon_LIBS ?
    set(DirectX11_LIBS d3d11 d3dcompiler)
    set(DirectX12_LIBS d3d12 dxgi ${DXC_STATIC})

    set(SuperNovaEngine_LIBS_PRIVATE ${SuperNovaEngine_LIBS_PRIVATE}
        ${DirectX11_LIBS}
        ${DirectX12_LIBS}
    )
endif()

# --------------------- SuperNova-Editor ---------------------
set(SuperNovaEditor_LIBS_PRIVATE
    imgui::imgui
)


# ----------------------------------------------------------------------------------------------------
# ------------------------------------- SuperNova-Engine sources -------------------------------------
# ----------------------------------------------------------------------------------------------------
set(SuperNovaEngine_DIR         ${SuperNova_DIR}/Engine)
set(SuperNovaEngine_PUBLIC_DIR  ${SuperNovaEngine_DIR}/public)
set(SuperNovaEngine_PRIVATE_DIR ${SuperNovaEngine_DIR}/private)

set(SuperNovaEngine_SRC_DIR         ${SuperNovaEngine_DIR}/source)
set(SuperNovaEngine_INC_PUBLIC_DIR  ${SuperNovaEngine_PUBLIC_DIR}/Engine)
set(SuperNovaEngine_INC_PRIVATE_DIR ${SuperNovaEngine_PRIVATE_DIR}/Engine)

# ----------------------- Application ------------------------
set(Application_SRC_DIR        ${SuperNovaEngine_SRC_DIR}/Application)
set(Application_INC_PUBLIC_DIR ${SuperNovaEngine_INC_PUBLIC_DIR}/Application)

set(Application_SRC
    ${Application_SRC_DIR}/Application.cpp
    ${Application_SRC_DIR}/Window.cpp
)
set(Application_INC_PUBLIC
    ${Application_INC_PUBLIC_DIR}/Application.hpp
    ${Application_INC_PUBLIC_DIR}/IApplicationLayer.hpp
    ${Application_INC_PUBLIC_DIR}/Window.hpp
)

# -------------------------- Assets --------------------------
set(Assets_SRC_DIR        ${SuperNovaEngine_SRC_DIR}/Assets)
set(Assets_INC_PUBLIC_DIR ${SuperNovaEngine_INC_PUBLIC_DIR}/Assets)

set(Assets_SRC
    ${Assets_SRC_DIR}/AssetDatabase.cpp
    ${Assets_SRC_DIR}/Material.cpp
    ${Assets_SRC_DIR}/Mesh.cpp
    ${Assets_SRC_DIR}/Model.cpp
    ${Assets_SRC_DIR}/Shader.cpp
    ${Assets_SRC_DIR}/Texture.cpp
)
set(Assets_INC_PUBLIC
    ${Assets_INC_PUBLIC_DIR}/AssetDatabase.hpp
    ${Assets_INC_PUBLIC_DIR}/Material.hpp
    ${Assets_INC_PUBLIC_DIR}/Mesh.hpp
    ${Assets_INC_PUBLIC_DIR}/Model.hpp
    ${Assets_INC_PUBLIC_DIR}/Shader.hpp
    ${Assets_INC_PUBLIC_DIR}/Texture.hpp
)

# ------------------------ Components ------------------------
set(Components_SRC_DIR        ${SuperNovaEngine_SRC_DIR}/Components)
set(Components_INC_PUBLIC_DIR ${SuperNovaEngine_INC_PUBLIC_DIR}/Components)

set(Components_SRC
    ${Components_SRC_DIR}/Camera.cpp
    ${Components_SRC_DIR}/CameraController.cpp
    ${Components_SRC_DIR}/ComponentFactory.cpp
    ${Components_SRC_DIR}/MeshRenderer.cpp
    ${Components_SRC_DIR}/Transform.cpp
)
set(Components_INC_PUBLIC
    ${Components_INC_PUBLIC_DIR}/Camera.hpp
    ${Components_INC_PUBLIC_DIR}/CameraController.hpp
    ${Components_INC_PUBLIC_DIR}/Component.hpp
    ${Components_INC_PUBLIC_DIR}/ComponentFactory.hpp
    ${Components_INC_PUBLIC_DIR}/MeshRenderer.hpp
    ${Components_INC_PUBLIC_DIR}/Transform.hpp
)

# --------------------------- Core ---------------------------
set(Core_SRC_DIR        ${SuperNovaEngine_SRC_DIR}/Core)
set(Core_INC_PUBLIC_DIR ${SuperNovaEngine_INC_PUBLIC_DIR}/Core)

set(Core_SRC
    ${Core_SRC_DIR}/Log.cpp
)
set(Core_INC_PUBLIC
    ${Core_INC_PUBLIC_DIR}/Assert.hpp
    ${Core_INC_PUBLIC_DIR}/Core.hpp
    ${Core_INC_PUBLIC_DIR}/Log.hpp
)

# -------------------------- Engine --------------------------
set(Engine_SRC
    ${SuperNovaEngine_SRC_DIR}/Engine.cpp
)
set(Engine_INC_PUBLIC
    ${SuperNovaEngine_INC_PUBLIC_DIR}/Engine.hpp
)

# -------------------------- Entity --------------------------
set(Entity_SRC_DIR        ${SuperNovaEngine_SRC_DIR}/Entity)
set(Entity_INC_PUBLIC_DIR ${SuperNovaEngine_INC_PUBLIC_DIR}/Entity)

set(Entity_SRC
    ${Entity_SRC_DIR}/GameObject.cpp
)
set(Entity_INC_PUBLIC
    ${Entity_INC_PUBLIC_DIR}/GameObject.hpp
)

# -------------------------- Input ---------------------------
set(Input_SRC_DIR        ${SuperNovaEngine_SRC_DIR}/Input)
set(Input_INC_PUBLIC_DIR ${SuperNovaEngine_INC_PUBLIC_DIR}/Input)

set(Input_SRC
    ${Input_SRC_DIR}/Cursor.cpp
    ${Input_SRC_DIR}/Keyboard.cpp
    ${Input_SRC_DIR}/Mouse.cpp
)
set(Input_INC_PUBLIC
    ${Input_INC_PUBLIC_DIR}/Cursor.hpp
    ${Input_INC_PUBLIC_DIR}/Input.hpp
    ${Input_INC_PUBLIC_DIR}/Keyboard.hpp
    ${Input_INC_PUBLIC_DIR}/KeyboardKey.hpp
    ${Input_INC_PUBLIC_DIR}/Mouse.hpp
)

# ------------------------- Renderer -------------------------
set(Renderer_SRC_DIR         ${SuperNovaEngine_SRC_DIR}/Renderer)
set(Renderer_INC_PUBLIC_DIR  ${SuperNovaEngine_INC_PUBLIC_DIR}/Renderer)
set(Renderer_INC_PRIVATE_DIR ${SuperNovaEngine_INC_PRIVATE_DIR}/Renderer)

# ------------ OpenGL ------------
set(OpenGL_SRC_DIR ${Renderer_SRC_DIR}/OpenGL)
set(OpenGL_INC_PRIVATE_DIR ${Renderer_INC_PRIVATE_DIR}/OpenGL)
set(OpenGL_SRC
    ${OpenGL_SRC_DIR}/GLBackend.cpp
    ${OpenGL_SRC_DIR}/GLBuffer.cpp
    ${OpenGL_SRC_DIR}/GLShader.cpp
    ${OpenGL_SRC_DIR}/GLTexture.cpp
)
set(OpenGL_INC_PRIVATE
    ${OpenGL_INC_PRIVATE_DIR}/GLBackend.hpp
    ${OpenGL_INC_PRIVATE_DIR}/GLBuffer.hpp
    ${OpenGL_INC_PRIVATE_DIR}/GLShader.hpp
    ${OpenGL_INC_PRIVATE_DIR}/GLTexture.hpp
)

# ---------- DirectX 11 ----------
set(DirectX11_SRC_DIR ${Renderer_SRC_DIR}/DirectX11)
set(DirectX11_INC_DIR_PRIVATE ${Renderer_INC_PRIVATE_DIR}/DirectX11)
set(DirectX11_SRC
    ${DirectX11_SRC_DIR}/DX11Backend.cpp
)
set(DirectX11_INC_PRIVATE
    ${DirectX11_INC_DIR_PRIVATE}/DX11Backend.hpp
)

# ---------- DirectX 12 ----------
set(DirectX12_SRC_DIR ${Renderer_SRC_DIR}/DirectX12)
set(DirectX12_INC_DIR_PRIVATE ${Renderer_INC_PRIVATE_DIR}/DirectX12)
set(DirectX12_SRC
    ${DirectX12_SRC_DIR}/DX12Backend.cpp
    ${DirectX12_SRC_DIR}/DX12ShaderCompiler.cpp
)
set(DirectX12_INC_PRIVATE
    ${DirectX12_INC_DIR_PRIVATE}/DX12Backend.hpp
    ${DirectX12_INC_DIR_PRIVATE}/DX12ShaderCompiler.hpp
)


set(Renderer_SRC
    ${Renderer_SRC_DIR}/Renderer.cpp
    ${OpenGL_SRC}
)
set(Renderer_INC_PUBLIC
    ${Renderer_INC_PUBLIC_DIR}/IRendererBackend.hpp
    ${Renderer_INC_PUBLIC_DIR}/Renderer.hpp
    ${Renderer_INC_PUBLIC_DIR}/RenderTypes.hpp
)
set(Renderer_INC_PRIVATE
    ${OpenGL_INC_PRIVATE}
)

if(SNV_PLATFORM_WINDOWS)
    set(Renderer_SRC ${Renderer_SRC}
        ${DirectX11_SRC}
        ${DirectX12_SRC}
    )
    set(Renderer_INC_PRIVATE ${Renderer_INC_PRIVATE}
        ${DirectX11_INC_PRIVATE}
        ${DirectX12_INC_PRIVATE}
    )
endif()

# -------------------------- Utils ---------------------------
set(Utils_SRC_DIR        ${SuperNovaEngine_SRC_DIR}/Utils)
set(Utils_INC_PUBLIC_DIR ${SuperNovaEngine_INC_PUBLIC_DIR}/Utils)

set(Utils_SRC
    ${Utils_SRC_DIR}/Time.cpp
)
set(Utils_INC_PUBLIC
    ${Utils_INC_PUBLIC_DIR}/Time.hpp
    # ${Utils_INC_PUBLIC_DIR}/Singleton.hpp
)


set(SuperNovaEngine_SRC
    ${Application_SRC}
    ${Assets_SRC}
    ${Components_SRC}
    ${Core_SRC}
    ${Engine_SRC}
    ${Entity_SRC}
    ${Input_SRC}
    ${Renderer_SRC}
    ${Utils_SRC}
)
set(SuperNovaEngine_INC_PUBLIC
    ${Application_INC_PUBLIC}
    ${Assets_INC_PUBLIC}
    ${Components_INC_PUBLIC}
    ${Core_INC_PUBLIC}
    ${Engine_INC_PUBLIC}
    ${Entity_INC_PUBLIC}
    ${Input_INC_PUBLIC}
    ${Renderer_INC_PUBLIC}
    ${Utils_INC_PUBLIC}
)
set(SuperNovaEngine_INC_PRIVATE
    ${Renderer_INC_PRIVATE}
)


# ----------------------------------------------------------------------------------------------------
# ------------------------------------- SuperNova-Editor sources -------------------------------------
# ----------------------------------------------------------------------------------------------------
set(SuperNovaEditor_DIR         ${SuperNova_DIR}/Editor)
set(SuperNovaEditor_PRIVATE_DIR ${SuperNovaEditor_DIR}/private)

set(SuperNovaEditor_SRC_DIR         ${SuperNovaEditor_DIR}/source)
set(SuperNovaEditor_INC_PRIVATE_DIR ${SuperNovaEditor_PRIVATE_DIR}/Editor)

# -------------------------- Editor --------------------------
set(Editor_SRC
    ${SuperNovaEditor_SRC_DIR}/Editor.cpp
)
set(Editor_INC_PURIVATE
    ${SuperNovaEditor_INC_PRIVATE_DIR}/Editor.hpp
)


set(SuperNovaEditor_SRC
    ${SuperNovaEditor_SRC_DIR}/main.cpp
    ${Editor_SRC}
)
set(SuperNovaEditor_INC_PRIVATE
    ${Editor_INC_PURIVATE}
)


# ----------------------------------------------------------------------------------------------------
# ---------------------------------------- SuperNova targets -----------------------------------------
# ----------------------------------------------------------------------------------------------------

# --------------------- SuperNova-Engine ---------------------
add_library(SuperNovaEngine STATIC
    ${SuperNovaEngine_SRC}
    ${SuperNovaEngine_INC_PUBLIC}
    ${SuperNovaEngine_INC_PRIVATE}
)
target_link_libraries(SuperNovaEngine
    PUBLIC
        ${SuperNovaEngine_LIBS_PUBLIC}
    PRIVATE
        ${SuperNovaEngine_LIBS_PRIVATE}
)
target_include_directories(SuperNovaEngine
    PUBLIC
        ${SuperNovaEngine_PUBLIC_DIR}
    PRIVATE
        ${SuperNovaEngine_PRIVATE_DIR}
        ${LIBS_INCLUDE_DIR}
)

# --------------------- SuperNova-Editor ---------------------
add_executable(SuperNovaEditor
    ${SuperNovaEditor_SRC}
    ${SuperNovaEditor_INC_PRIVATE}
)
target_link_libraries(SuperNovaEditor
    PRIVATE
        SuperNovaEngine
        ${SuperNovaEditor_LIBS_PRIVATE}
)
target_include_directories(SuperNovaEditor
    PRIVATE
        ${SuperNovaEditor_PRIVATE_DIR}
)

if(SNV_PLATFORM_WINDOWS)
    add_custom_command(TARGET SuperNovaEditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DXC_SHARED} $<TARGET_FILE_DIR:SuperNovaEditor>
    )
endif()
