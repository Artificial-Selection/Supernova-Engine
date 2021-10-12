# SuperNova Engine

## Build

Requirements:
- [Python 3.9.7](https://www.python.org/downloads/)
  - Conan 1.41.0
- [CMake 3.21.3](https://cmake.org/download/)
- [Ninja 1.10.2](https://github.com/ninja-build/ninja/releases)
- [Vulkan SDK 1.2.189.2](https://vulkan.lunarg.com/sdk/home)  
  Select "(Optional) Debuggable Shader API Libraries - 64 bit" during installation

Run `conan-init.ps1` powershell script

Compile:
```
cmake . --preset <PRESET_NAME>
cmake --build --preset <PRESET_NAME>
```
Where `PRESET_NAME` is one of the following:
- msvc_Debug
- msvc_Release
- visual-studio

## Run

Download [Sponza](https://github.com/jimmiebergmann/Sponza) and put it in the `assets/models/Sponza` folder
