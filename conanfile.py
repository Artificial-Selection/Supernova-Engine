from conans import ConanFile

class SuperNovaEngine(ConanFile):
    name = 'SuperNova-Engine'
    version = '0.1.0'
    settings = "os", "arch", "compiler", "build_type"

    generators = "cmake_find_package_multi"

    requires = [
        'glad/0.1.34',
        'glfw/3.3.4',
        'glm/0.9.9.8',
        'entt/3.7.1',
        'spdlog/1.8.5'
    ]
