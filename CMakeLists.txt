cmake_minimum_required(VERSION 3.27)
project(RollerCoasterGL LANGUAGES CXX C)

# ---------------- Basics ----------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# vcpkg
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "" FORCE)
endif()

# Warnings
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ---------------- Third-party ----------------
# GLAD
add_library(glad STATIC thirdparty/glad/glad.c)
target_include_directories(glad PUBLIC thirdparty/glad/)

# ImGui
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/thirdparty/imgui")
if(EXISTS "${IMGUI_DIR}/imgui.cpp")
    file(GLOB IMGUI_SOURCES
            "${IMGUI_DIR}/*.cpp"
            "${IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
            "${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp")
    add_library(imgui STATIC ${IMGUI_SOURCES})
    target_include_directories(imgui PUBLIC
            ${IMGUI_DIR}
            ${IMGUI_DIR}/backends
    )
    set(HAVE_IMGUI TRUE)
else()
    set(HAVE_IMGUI FALSE)
endif()

# ---------------- Packages ----------------
find_package(glfw3 CONFIG REQUIRED)   # vcpkg: glfw3
find_package(glm CONFIG REQUIRED)     # vcpkg: glm

# Linux
if(UNIX AND NOT APPLE)
    find_package(OpenGL REQUIRED)
endif()

# ---------------- Sources ----------------

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

add_executable(RollerCoasterGL ${SRC_FILES})
add_custom_command(TARGET RollerCoasterGL POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:RollerCoasterGL>/assets
        COMMENT "Copying assets/ next to the executable"
)
# ---------------- Includes ----------------
target_include_directories(RollerCoasterGL PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/thirdparty/glad/
)

# ---------------- Link ----------------
target_link_libraries(RollerCoasterGL PRIVATE
        glad
        glfw
        glm::glm
)

if(HAVE_IMGUI)
    target_link_libraries(RollerCoasterGL PRIVATE imgui)
    target_compile_definitions(RollerCoasterGL PRIVATE RCG_HAVE_IMGUI=1)
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(RollerCoasterGL PRIVATE OpenGL::GL dl)
endif()