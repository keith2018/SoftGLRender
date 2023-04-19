cmake_minimum_required(VERSION 3.10)
project(SoftGLRender)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(CMAKE_CXX_STANDARD 11)

set(TARGET_NAME SoftGLRender)
set(THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party")

# enable SIMD
add_definitions("-DSOFTGL_SIMD_OPT")

# debug
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions("-DDEBUG")
endif ()

find_program(CCACHE "ccache")
if (CCACHE)
    message(STATUS "find ccache")
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE})
endif (CCACHE)

include_directories(
        "${THIRD_PARTY_DIR}/glfw/include"
        "${THIRD_PARTY_DIR}/glad/include"
        "${THIRD_PARTY_DIR}/glm"
        "${THIRD_PARTY_DIR}/assimp/include"
        "${THIRD_PARTY_DIR}/assimp/contrib"
        "${THIRD_PARTY_DIR}/imgui"
        "${THIRD_PARTY_DIR}/json11"
        "${THIRD_PARTY_DIR}/md5"
        "${THIRD_PARTY_DIR}/stb/include"
        "${THIRD_PARTY_DIR}/vulkan/include"
        "${THIRD_PARTY_DIR}/glslang"
        "${THIRD_PARTY_DIR}/vma"
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

# assimp
set(BUILD_SHARED_LIBS OFF)
set(ASSIMP_NO_EXPORT ON)
set(ASSIMP_BUILD_TESTS OFF)
set(ASSIMP_INSTALL OFF)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
set(ASSIMP_BUILD_ZLIB ON)

set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT FALSE)
set(ASSIMP_BUILD_OBJ_IMPORTER TRUE)
set(ASSIMP_BUILD_GLTF_IMPORTER TRUE)

add_subdirectory(${THIRD_PARTY_DIR}/assimp)


# glslang
set(SKIP_GLSLANG_INSTALL ON)
set(ENABLE_SPVREMAPPER OFF)
set(ENABLE_GLSLANG_BINARIES OFF)
set(ENABLE_CTEST OFF)
set(BUILD_TESTING OFF)
set(USE_CCACHE ON)
add_subdirectory("${THIRD_PARTY_DIR}/glslang")

# main src
file(GLOB SOFTGL_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Base/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Render/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Render/Software/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Render/OpenGL/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Render/Vulkan/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Render/Vulkan/SPIRV/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Viewer/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Viewer/Shader/GLSL/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Viewer/Shader/Software/*.cpp
        )

# imgui src
file(GLOB IMGUI_SRC
        ${THIRD_PARTY_DIR}/imgui/imgui/*.cpp
        )

add_executable(${TARGET_NAME}
        "${SOFTGL_SRC}"
        "${IMGUI_SRC}"
        "${THIRD_PARTY_DIR}/glad/src/glad.c"
        "${THIRD_PARTY_DIR}/json11/json11.cpp"
        "${THIRD_PARTY_DIR}/md5/md5.c"
        )

set(LINK_LIBS
        assimp
        glslang
        glslang-default-resource-limits
        SPIRV
        )

if (WIN32)
    set(LINK_LIBS ${LINK_LIBS}
            "${THIRD_PARTY_DIR}/vulkan/lib-win-x64/vulkan-1.lib"
            )
    if (MSVC)
        set(LINK_LIBS ${LINK_LIBS}
                "${THIRD_PARTY_DIR}/glfw/lib-vc2022/glfw3.lib"
                "${THIRD_PARTY_DIR}/glfw/lib-vc2022/glfw3dll.lib"
                )
    else ()
        set(LINK_LIBS ${LINK_LIBS}
                "${THIRD_PARTY_DIR}/glfw/lib-mingw-w64/libglfw3.a"
                "${THIRD_PARTY_DIR}/glfw/lib-mingw-w64/libglfw3dll.a"
                )
    endif ()
endif ()

if (APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -framework Cocoa -framework OpenGL -framework IOKit")
    add_compile_definitions(GL_SILENCE_DEPRECATION)
    set(LINK_LIBS ${LINK_LIBS}
            "${THIRD_PARTY_DIR}/vulkan/lib-macos-universal/libvulkan.1.dylib"
            "${THIRD_PARTY_DIR}/glfw/lib-macos-universal/libglfw3.a"
            )
endif ()

if (UNIX AND NOT APPLE)
    find_package(OpenGL REQUIRED)
    set(LINK_LIBS ${LINK_LIBS}
            pthread
            glfw
            OpenGL::GL
            "${THIRD_PARTY_DIR}/vulkan/lib-linux-x64/libvulkan.so.1"
            ${CMAKE_DL_LIBS}
            )
endif ()

if (MSVC)
    target_compile_options(${TARGET_NAME} PRIVATE $<$<BOOL:${MSVC}>:/arch:AVX2 /std:c++11>)
else ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2 -mfma -O3")
endif ()

target_link_libraries(${TARGET_NAME} ${LINK_LIBS})

# output dir
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)

# copy assets
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:${TARGET_NAME}>/assets

        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Viewer/Shader/GLSL $<TARGET_FILE_DIR:${TARGET_NAME}>/shaders/GLSL

        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/cache $<TARGET_FILE_DIR:${TARGET_NAME}>/cache
        )
