cmake_minimum_required(VERSION 3.20)
project(RollerCoasterGL LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# -------- OpenGL + GLFW ----------
find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

# -------- GLAD ----------
add_library(glad STATIC thirdparty/glad/glad.c)

target_include_directories(glad PUBLIC
        ${CMAKE_SOURCE_DIR}/thirdparty/glad
        ${CMAKE_SOURCE_DIR}/thirdparty
)

# -------- ImGui ----------
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/thirdparty/imgui)
set(IMGUI_SOURCES
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
)

target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD)
target_link_libraries(imgui PUBLIC glfw OpenGL::GL glad)

# -------- src ----------
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")

add_executable(RollerCoasterGL ${SRC_FILES})
target_include_directories(RollerCoasterGL PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Linkowanie
target_link_libraries(RollerCoasterGL PRIVATE imgui glfw glad OpenGL::GL)

# assets
add_custom_command(TARGET RollerCoasterGL POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:RollerCoasterGL>/assets
        COMMENT "Copying assets/ next to the executable"
)
