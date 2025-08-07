cmake_minimum_required(VERSION 3.31)
project(RollerCoasterGL)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

add_library(glad STATIC thirdparty/glad/glad.c)
target_include_directories(glad PUBLIC thirdparty/glad)
add_library(imgui STATIC thirdparty/imgui/imgui.cpp
        thirdparty/imgui/imgui_draw.cpp
        thirdparty/imgui/imgui_widgets.cpp
        thirdparty/imgui/imgui_tables.cpp
        thirdparty/imgui/imgui_demo.cpp
        thirdparty/imgui/backends/imgui_impl_glfw.cpp
        thirdparty/imgui/backends/imgui_impl_opengl3.cpp)
target_include_directories(imgui PUBLIC thirdparty/imgui)

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

add_executable(RollerCoasterGL main.cpp
        SimplexNoise.cpp
        Terrain.cpp
        FreeFlyCam.cpp
        Spline.cpp
)

target_link_libraries(RollerCoasterGL glad glfw glm::glm imgui)
target_include_directories(RollerCoasterGL PRIVATE thirdparty/glad thirdparty/imgui thirdparty/imgui/backends)