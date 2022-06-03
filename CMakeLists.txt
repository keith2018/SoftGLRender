cmake_minimum_required(VERSION 3.10)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
project(SoftGLRender)

set(CMAKE_CXX_STANDARD 11)
set(THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party")

# texture tiled memory
add_definitions("-DSOFTGL_TEXTURE_TILED")

# SIMD
add_definitions("-DSOFTGL_SIMD_OPT")

include_directories(
        "${THIRD_PARTY_DIR}/glfw/include"
        "${THIRD_PARTY_DIR}/glad/include"
        "${THIRD_PARTY_DIR}/glm"
        "${THIRD_PARTY_DIR}/assimp/include"
        "${THIRD_PARTY_DIR}/assimp/contrib"
        "${THIRD_PARTY_DIR}/imgui"
        "${THIRD_PARTY_DIR}/json11"
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


# main src
file(GLOB SOFTGL_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/app/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/utils/*.cpp
        )

# imgui src
file(GLOB IMGUI_SRC
        ${THIRD_PARTY_DIR}/imgui/imgui/*.cpp
        )

add_executable(SoftGLRender
        "${SOFTGL_SRC}"
        "${IMGUI_SRC}"
        "${THIRD_PARTY_DIR}/glad/src/glad.c"
        "${THIRD_PARTY_DIR}/json11/json11.cpp"
        src/main.cpp
        )

if (WIN32)
    if (MSVC)
        target_link_libraries(SoftGLRender
                assimp
                "${THIRD_PARTY_DIR}/glfw/lib-vc2022/glfw3.lib"
                "${THIRD_PARTY_DIR}/glfw/lib-vc2022/glfw3dll.lib"
                )
    else ()
        target_link_libraries(SoftGLRender
                assimp
                "${THIRD_PARTY_DIR}/glfw/lib-mingw-w64/libglfw3.a"
                "${THIRD_PARTY_DIR}/glfw/lib-mingw-w64/libglfw3dll.a"
                )
    endif ()
endif ()

if (APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -framework Cocoa -framework OpenGL -framework IOKit")
    add_compile_definitions(GL_SILENCE_DEPRECATION)
    target_link_libraries(SoftGLRender
            assimp
            "${THIRD_PARTY_DIR}/glfw/lib-macos-universal/libglfw3.a"
            )
endif ()

if (MSVC)
    target_compile_options(SoftGLRender PRIVATE $<$<BOOL:${MSVC}>:/arch:AVX2 /std:c++11>)
else ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2 -mfma -O3")
endif ()


# output dir
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)

# copy assets
add_custom_command(TARGET SoftGLRender POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:SoftGLRender>/assets)
