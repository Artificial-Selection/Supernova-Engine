# Thanks to:
# - https://github.com/EQMG/Acid/blob/master/CMake/FindGlslang.cmake
# - https://github.com/v1993/GLSLang-cmake/blob/master/FindGLSLang.cmake

include(SelectLibraryConfigurations)
include(FindPackageHandleStandardArgs)


set(VULKAN_SDK_HINT $ENV{VULKAN_SDK}/Lib)
mark_as_advanced(${VULKAN_SDK_HINT})


function(_GLSLang_find_libraries GLSLANG_LIB_NAMES)

    foreach(GLSLANG_LIB_NAME ${GLSLANG_LIB_NAMES})
        find_library(${GLSLANG_LIB_NAME}_LIBRARY_RELEASE NAMES ${GLSLANG_LIB_NAME}  HINTS ${VULKAN_SDK_HINT})
        find_library(${GLSLANG_LIB_NAME}_LIBRARY_DEBUG   NAMES ${GLSLANG_LIB_NAME}d HINTS ${VULKAN_SDK_HINT})

        select_library_configurations(${GLSLANG_LIB_NAME})

        find_package_handle_standard_args(glslang
             DEFAULT_MSG
             ${GLSLANG_LIB_NAME}_LIBRARY
        )

        #- Add library
        add_library(glslang::${GLSLANG_LIB_NAME} STATIC IMPORTED)
        #-- Release
        set_property(TARGET glslang::${GLSLANG_LIB_NAME} APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
        set_target_properties(glslang::${GLSLANG_LIB_NAME} PROPERTIES
            IMPORTED_LOCATION_RELEASE "${${GLSLANG_LIB_NAME}_LIBRARY_RELEASE}"
        )
        #-- Debug
        set_property(TARGET glslang::${GLSLANG_LIB_NAME} APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
        set_target_properties(glslang::${GLSLANG_LIB_NAME} PROPERTIES
            IMPORTED_LOCATION_DEBUG "${${GLSLANG_LIB_NAME}_LIBRARY_DEBUG}"
        )

        mark_as_advanced(${GLSLANG_LIB_NAME}_LIBRARY)
    endforeach()

endfunction()


# WHY THE FUCK IT CANNOT FIND OSDependent BUT CAN FIND OSDEPENDENT
_GLSLang_find_libraries(
    "glslang;MachineIndependent;OSDEPENDENT;OGLCompiler;GenericCodeGen;SPIRV;SPIRV-Tools;SPIRV-Tools-opt;"
)
