cmake_minimum_required(VERSION 3.21)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(vulkan_starter VERSION 0.0.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT DEFINED OPT_LEVEL)
    set(OPT_LEVEL 2)
endif()

include_directories(
    $ENV{VULKAN_SDK}/include
)

# Global flags.
add_compile_options("-O${OPT_LEVEL}")
add_compile_options("-g")
add_compile_options("-fno-exceptions")
add_compile_options("-fmacro-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}/=")

set(SOURCES
    sources/main.cxx
    sources/utils.cxx
    sources/vma_impl.cxx
)

add_executable(${PROJECT_NAME} ${SOURCES})

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

# === VMA === #
add_subdirectory(libraries/VulkanMemoryAllocator)
target_link_libraries(${PROJECT_NAME} PRIVATE VulkanMemoryAllocator)

# === VOLK === #
add_subdirectory(libraries/volk)
target_link_libraries(${PROJECT_NAME} PRIVATE volk)

# === Vulkan === #
find_package(Vulkan REQUIRED)
target_compile_definitions(${PROJECT_NAME} PRIVATE VK_NO_PROTOTYPES)
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)

# === Shaders Target === #
set(SHADERS
    the_shader.slang
)

set(SHADER_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/sources/shaders")
set(SHADER_OUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders")

foreach(shader ${SHADERS})
    get_filename_component(shader_name ${shader} NAME)
    get_filename_component(shader_ext ${shader} EXT)

    set(src "${SHADER_SRC_DIR}/${shader_name}")
    set(out "${SHADER_OUT_DIR}/${shader_name}.spv")

    # Automatically selects `slangc` or `glslc` depending on file extension.
    if(shader_ext STREQUAL ".slang")
        set(SHADER_COMPILER slangc)
    elseif(shader_ext STREQUAL ".vert" OR shader_ext STREQUAL ".frag" OR shader_ext STREQUAL ".comp")
        set(SHADER_COMPILER glslc)
    else()
        message(FATAL_ERROR "Unknown file extension for shader: ${shader} (.vert/.frag/.comp for GLSL, .slang for Slang)")
    endif()

    add_custom_command(
        OUTPUT ${out}
        COMMAND ${SHADER_COMPILER} ${src} -o ${out}
        DEPENDS ${src}
        COMMENT "Compiling SPIR-V binary ${shader_name} from Slang"
        VERBATIM
    )

    list(APPEND SPIRV_SHADERS ${out})
endforeach()

add_custom_target(shaders ALL
    DEPENDS ${SPIRV_SHADERS}
)

# === MacOS-specific stuff === #
if(APPLE)
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
