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


set(BUILD_DIR ${PROJECT_SOURCE_DIR}/build)


# ----------------------------------------------------------------------------------------------------
# ------------------------------------------- Conan setup --------------------------------------------
# ----------------------------------------------------------------------------------------------------
set(ENV{CONAN_USER_HOME} ${PROJECT_SOURCE_DIR})

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
    message(FATAL_ERROR "The file ${CONAN_BUILD_INFO_FILE} doesn't exist, you're fucked, comrade")
endif()


# ----------------------------------------------------------------------------------------------------
# ------------------------------------------ 3rdparty libs -------------------------------------------
# ----------------------------------------------------------------------------------------------------
set(3RDPARTY_LIBS_DIR ${PROJECT_SOURCE_DIR}/libs)
set(3RDPARTY_LIBS_INCLUDE_DIR ${3RDPARTY_LIBS_DIR}/include)
set(3RDPARTY_LIBS_STATIC_DIR ${3RDPARTY_LIBS_DIR}/lib)
set(3RDPARTY_LIBS_SHARED_DIR ${3RDPARTY_LIBS_DIR}/shared)

# --------------------------- DXC ----------------------------
set(DXC_STATIC ${3RDPARTY_LIBS_STATIC_DIR}/dxcompiler.lib)
set(DXC_SHARED
    ${3RDPARTY_LIBS_SHARED_DIR}/dxcompiler.dll
    ${3RDPARTY_LIBS_SHARED_DIR}/dxil.dll
)

# ------------------------ Conan libs ------------------------
find_package(assimp REQUIRED)
find_package(EnTT   REQUIRED)
find_package(glad   REQUIRED)
find_package(glfw3  REQUIRED)
find_package(glm    REQUIRED)
find_package(imgui  REQUIRED)
find_package(spdlog REQUIRED)
find_package(stb    REQUIRED)


set(3RDPARTY_LIBS
    assimp::assimp
    EnTT::EnTT
    glad::glad
    glfw::glfw
    glm::glm
    imgui::imgui
    spdlog::spdlog
    stb::stb
)

if(SNV_PLATFORM_WINDOWS)
    # NOTE: d3dcompiler/dxgi should be in DirectXCommon_LIBS ?
    set(DirectX11_LIBS d3d11 d3dcompiler)
    set(DirectX12_LIBS d3d12 dxgi ${DXC_STATIC})

    set(3RDPARTY_LIBS ${3RDPARTY_LIBS}
        ${DirectX11_LIBS}
        ${DirectX12_LIBS}
    )
endif()


# ----------------------------------------------------------------------------------------------------
# ------------------------------------------ Engine sources ------------------------------------------
# ----------------------------------------------------------------------------------------------------
set(MAIN_SRC_DIR ${PROJECT_SOURCE_DIR}/source)

# --------------------------- App ----------------------------
set(Application_SRC_DIR ${MAIN_SRC_DIR}/Application)
set(Application_SRC
    ${Application_SRC_DIR}/Application.hpp
    ${Application_SRC_DIR}/Application.cpp
    ${Application_SRC_DIR}/IApplicationLayer.hpp
    ${Application_SRC_DIR}/Window.hpp
    ${Application_SRC_DIR}/Window.cpp
)

# -------------------------- Assets --------------------------
set(Assets_SRC_DIR ${MAIN_SRC_DIR}/Assets)
set(Assets_SRC
    ${Assets_SRC_DIR}/AssetDatabase.hpp
    ${Assets_SRC_DIR}/AssetDatabase.cpp
    ${Assets_SRC_DIR}/Material.hpp
    ${Assets_SRC_DIR}/Material.cpp
    ${Assets_SRC_DIR}/Mesh.hpp
    ${Assets_SRC_DIR}/Mesh.cpp
    ${Assets_SRC_DIR}/Model.hpp
    ${Assets_SRC_DIR}/Model.cpp
    ${Assets_SRC_DIR}/Shader.hpp
    ${Assets_SRC_DIR}/Shader.cpp
    ${Assets_SRC_DIR}/Texture.hpp
    ${Assets_SRC_DIR}/Texture.cpp
)

# ------------------------ Components ------------------------
set(Components_SRC_DIR ${MAIN_SRC_DIR}/Components)
set(Components_SRC
    ${Components_SRC_DIR}/Camera.hpp
    ${Components_SRC_DIR}/Camera.cpp
    ${Components_SRC_DIR}/CameraController.hpp
    ${Components_SRC_DIR}/CameraController.cpp
    ${Components_SRC_DIR}/Component.hpp
    ${Components_SRC_DIR}/ComponentFactory.hpp
    ${Components_SRC_DIR}/ComponentFactory.cpp
    ${Components_SRC_DIR}/MeshRenderer.hpp
    ${Components_SRC_DIR}/MeshRenderer.cpp
    ${Components_SRC_DIR}/Transform.hpp
    ${Components_SRC_DIR}/Transform.cpp
)

# --------------------------- Core ---------------------------
set(Core_SRC_DIR ${MAIN_SRC_DIR}/Core)
set(Core_SRC
    ${Core_SRC_DIR}/Assert.hpp
    ${Core_SRC_DIR}/Core.hpp
    ${Core_SRC_DIR}/Log.hpp
    ${Core_SRC_DIR}/Log.cpp
)

# -------------------------- Engine --------------------------
set(Engine_SRC_DIR ${MAIN_SRC_DIR}/Engine)
set(Engine_SRC
    ${Engine_SRC_DIR}/Engine.hpp
    ${Engine_SRC_DIR}/Engine.cpp
)

# -------------------------- Entity --------------------------
set(Entity_SRC_DIR ${MAIN_SRC_DIR}/Entity)
set(Entity_SRC
    ${Entity_SRC_DIR}/GameObject.hpp
    ${Entity_SRC_DIR}/GameObject.cpp
)

# -------------------------- Input ---------------------------
set(Input_SRC_DIR ${MAIN_SRC_DIR}/Input)
set(Input_SRC
    ${Input_SRC_DIR}/Cursor.hpp
    ${Input_SRC_DIR}/Cursor.cpp
    ${Input_SRC_DIR}/Input.hpp
    ${Input_SRC_DIR}/Keyboard.hpp
    ${Input_SRC_DIR}/Keyboard.cpp
    ${Input_SRC_DIR}/KeyboardKey.hpp
    ${Input_SRC_DIR}/Mouse.hpp
    ${Input_SRC_DIR}/Mouse.cpp
)

# ------------------------- Renderer -------------------------
set(Renderer_SRC_DIR ${MAIN_SRC_DIR}/Renderer)

set(DirectX11_SRC_DIR ${Renderer_SRC_DIR}/DirectX11)
set(DirectX11_SRC
    ${DirectX11_SRC_DIR}/DX11Backend.hpp
    ${DirectX11_SRC_DIR}/DX11Backend.cpp
)

set(DirectX12_SRC_DIR ${Renderer_SRC_DIR}/DirectX12)
set(DirectX12_SRC
    ${DirectX12_SRC_DIR}/DX12Backend.hpp
    ${DirectX12_SRC_DIR}/DX12Backend.cpp
    ${DirectX12_SRC_DIR}/DX12ShaderCompiler.hpp
    ${DirectX12_SRC_DIR}/DX12ShaderCompiler.cpp
)

set(OpenGL_SRC_DIR ${Renderer_SRC_DIR}/OpenGL)
set(OpenGL_SRC
    ${OpenGL_SRC_DIR}/GLBackend.hpp
    ${OpenGL_SRC_DIR}/GLBackend.cpp
    ${OpenGL_SRC_DIR}/GLBuffer.hpp
    ${OpenGL_SRC_DIR}/GLBuffer.cpp
    ${OpenGL_SRC_DIR}/GLShader.hpp
    ${OpenGL_SRC_DIR}/GLShader.cpp
    ${OpenGL_SRC_DIR}/GLTexture.hpp
    ${OpenGL_SRC_DIR}/GLTexture.cpp
)

set(Renderer_SRC
    ${OpenGL_SRC}
    ${Renderer_SRC_DIR}/IRendererBackend.hpp
    ${Renderer_SRC_DIR}/Renderer.hpp
    ${Renderer_SRC_DIR}/Renderer.cpp
    ${Renderer_SRC_DIR}/RenderTypes.hpp
)

if(SNV_PLATFORM_WINDOWS)
    set(Renderer_SRC ${Renderer_SRC}
        ${DirectX11_SRC}
        ${DirectX12_SRC}
    )
endif()

# -------------------------- Utils ---------------------------
set(Utils_SRC_DIR ${MAIN_SRC_DIR}/Utils)
set(Utils_SRC
    ${Utils_SRC_DIR}/Time.hpp
    ${Utils_SRC_DIR}/Time.cpp
    # ${Utils_SRC_DIR}/Singleton.cpp
)


add_executable(SuperNova
    ${MAIN_SRC_DIR}/main.cpp
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
target_link_libraries(SuperNova PRIVATE ${3RDPARTY_LIBS})
target_include_directories(SuperNova PRIVATE
    ${MAIN_SRC_DIR}
    ${3RDPARTY_LIBS_INCLUDE_DIR}
)

if(SNV_PLATFORM_WINDOWS)
    add_custom_command(TARGET SuperNova POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DXC_SHARED} $<TARGET_FILE_DIR:SuperNova>
    )
endif()
