cmake_minimum_required(VERSION 3.21)
project(vulkan_starter VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT DEFINED OPT_LEVEL)
    set(OPT_LEVEL 2)
endif()

include_directories(
    $ENV{VULKAN_SDK}/include
)

set(SOURCES
    sources/main.cxx
    sources/utils.cxx
    sources/vma_impl.cxx
)

add_executable(${PROJECT_NAME} ${SOURCES})

# Global flags.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O${OPT_LEVEL} -g -fno-exceptions")

# Project-local flags.
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall -Wextra -Wconversion -Wno-missing-designated-field-initializers
)

# === GLFW === #
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(libraries/glfw)
target_compile_definitions(${PROJECT_NAME} PRIVATE GLFW_INCLUDE_VULKAN)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

# === GLM === #
add_subdirectory(libraries/glm)
target_compile_definitions(${PROJECT_NAME} PRIVATE
    GLM_FORCE_RADIANS
    GLM_FORCE_DEPTH_ZERO_TO_ONE
)
target_link_libraries(${PROJECT_NAME} PRIVATE glm)

# === VMA === #
add_subdirectory(libraries/VulkanMemoryAllocator)
target_link_libraries(${PROJECT_NAME} PRIVATE VulkanMemoryAllocator)

# === FASTGLTF === #
add_subdirectory(libraries/fastgltf)
target_compile_definitions(${PROJECT_NAME} PRIVATE FASTGLTF_HAS_CONCEPTS)
target_link_libraries(${PROJECT_NAME} PRIVATE fastgltf)

# === VOLK === #
add_subdirectory(libraries/volk)
target_link_libraries(${PROJECT_NAME} PRIVATE volk)

# === Vulkan === #
find_package(Vulkan REQUIRED)
target_compile_definitions(${PROJECT_NAME} PRIVATE VK_NO_PROTOTYPES)
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)

# === MacOS-specific stuff === #
if(APPLE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        GLFW_EXPOSE_NATIVE_COCOA
        # VK_USE_PLATFORM_MACOS_MVK
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreFoundation"
        "-framework CoreAudio"
        "-framework CoreHaptics"
        "-framework CoreMedia"
        "-framework AVFoundation"
        "-framework AudioToolbox"
        "-framework AudioUnit"
        "-framework CoreVideo"
        "-framework QuartzCore"
        "-framework Metal"
        "-framework ForceFeedback"
        "-framework GameController"
        "-framework Carbon"
        "-framework CoreServices"
        "-framework SystemConfiguration"
        "-framework UniformTypeIdentifiers"
    )
endif()

# set(SHADERS
#     test.vert
#     test.frag
# )
# 
# set(SHADER_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/sources/shaders")
# set(SHADER_OUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources/shaders")
# 
# foreach(shader ${SHADERS})
#     get_filename_component(shader_name ${shader} NAME)
#     set(src "${SHADER_SRC_DIR}/${shader_name}")
#     set(out "${SHADER_OUT_DIR}/${shader_name}.spv")
# 
#     add_custom_command(
#         OUTPUT ${out}
#         COMMAND glslc ${src} -o ${out}
#         DEPENDS ${src}
#         COMMENT "Compiling shader ${shader_name} -> ${shader_name}.spv"
#         VERBATIM
#     )
# 
#     list(APPEND SPIRV_SHADERS ${out})
# endforeach()
# 
# add_custom_target(shaders ALL
#     DEPENDS ${SPIRV_SHADERS}
# )
