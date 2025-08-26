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

# ---------------- Packages ----------------
find_package(imgui CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

# Linux
if(UNIX AND NOT APPLE)
    find_package(OpenGL REQUIRED)
endif()

# ---------------- Sources ----------------

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

add_executable(RollerCoasterGL ${SRC_FILES}
        src/gameplay/Car.cpp)
add_custom_command(TARGET RollerCoasterGL POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:RollerCoasterGL>/assets
        COMMENT "Copying assets/ next to the executable"
)
# ---------------- Includes ----------------
target_include_directories(RollerCoasterGL PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        SYSTEM
        ${CMAKE_SOURCE_DIR}/thirdparty/glad/
)

# ---------------- Link ----------------
target_link_libraries(RollerCoasterGL PRIVATE imgui::imgui glfw glad OpenGL::GL)

if(UNIX AND NOT APPLE)
    target_link_libraries(RollerCoasterGL PRIVATE imgui::imgui glfw glad OpenGL::GL)
endif()