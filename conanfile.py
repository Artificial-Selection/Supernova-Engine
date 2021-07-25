from conans import ConanFile

class SuperNovaEngine(ConanFile):
    name = 'SuperNova-Engine'
    version = '0.1.0'
    settings = "os", "arch", "compiler", "build_type"

    generators = "cmake_find_package_multi"

    requires = [
        'assimp/5.0.1',
        'entt/3.7.1',
        'glad/0.1.34@snv/stable',
        'glfw/3.3.4',
        'glm/0.9.9.8',
        'imgui/18311@snv/docking',
        'spdlog/1.8.5',
        'stb/20200203'
    ]
