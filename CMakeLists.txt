cmake_minimum_required(VERSION 3.10)
project(SoftGLRender)

set(CMAKE_CXX_STANDARD 11)

# texture tiled memory
add_definitions("-DSOFTGL_TEXTURE_TILED")

# SIMD
add_definitions("-DSOFTGL_SIMD_OPT")

include_directories(
        "${CMAKE_CURRENT_SOURCE_DIR}/glfw/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/glad/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/glm"
        "${CMAKE_CURRENT_SOURCE_DIR}/assimp/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/assimp/contrib"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui"
        "${CMAKE_CURRENT_SOURCE_DIR}/json11"
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

set(BUILD_SHARED_LIBS ON)
set(ASSIMP_NO_EXPORT ON)
set(ASSIMP_BUILD_TESTS OFF)
set(ASSIMP_INSTALL OFF)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF)

set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT FALSE)
set(ASSIMP_BUILD_OBJ_IMPORTER TRUE)
set(ASSIMP_BUILD_GLTF_IMPORTER TRUE)

set(ASSIMP_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(ASSIMP_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/assimp)

file(GLOB SOFTGL_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/app/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/utils/*.cpp
        )

file(GLOB IMGUI_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui/*.cpp
        )

add_executable(SoftGLRender
        "${SOFTGL_SRC}"
        "${IMGUI_SRC}"
        "${CMAKE_CURRENT_SOURCE_DIR}/glad/src/glad.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/json11/json11.cpp"
        src/main.cpp
        )

SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)

if (WIN32)
    if (MSVC)
        target_link_libraries(SoftGLRender
                assimp
                "${CMAKE_CURRENT_SOURCE_DIR}/glfw/lib-vc2022/glfw3.lib"
                "${CMAKE_CURRENT_SOURCE_DIR}/glfw/lib-vc2022/glfw3dll.lib"
                )
    else ()
        target_link_libraries(SoftGLRender
                assimp
                "${CMAKE_CURRENT_SOURCE_DIR}/glfw/lib-mingw-w64/libglfw3.a"
                "${CMAKE_CURRENT_SOURCE_DIR}/glfw/lib-mingw-w64/libglfw3dll.a"
                )
    endif ()
endif ()

if (APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -framework Cocoa -framework OpenGL -framework IOKit")
    add_compile_definitions(GL_SILENCE_DEPRECATION)
    target_link_libraries(SoftGLRender
            assimp
            "${CMAKE_CURRENT_SOURCE_DIR}/glfw/lib-macos-universal/libglfw3.a"
            )
endif ()

if (MSVC)
    target_compile_options(SoftGLRender PRIVATE $<$<BOOL:${MSVC}>:/arch:AVX2 /std:c++11>)
else ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2 -mfma -O3")
endif ()
