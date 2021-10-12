from conans import ConanFile

class SuperNovaEngine(ConanFile):
    name = 'SuperNova-Engine'
    version = '0.1.0'
    settings = "os", "arch", "compiler", "build_type"

    generators = "cmake_find_package_multi"

    requires = [
        'assimp/5.0.1@snv/stable',
          'entt/3.8.1@snv/stable',
          'glad/0.1.34@snv/stable',
          'glfw/3.3.4@snv/stable',
           'glm/0.9.9.8@snv/stable',
         'imgui/18410@snv/docking',
        'spdlog/1.9.2@snv/stable',
           'stb/cci.20210713@snv/stable'
    ]

    #default_options = (
    #    'spdlog:wchar_support= True'
    #)
