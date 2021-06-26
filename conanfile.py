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

    default_options = (
        'glad:shared=False',
        'glad:no_loader=False',
        'glad:spec=gl',
        'glad:gl_profile=core',
        'glad:gl_version=4.6'
    )
