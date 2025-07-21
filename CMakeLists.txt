cmake_minimum_required(VERSION 3.31)
project(RollerCoasterGL)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

add_executable(RollerCoasterGL main.cpp SimplexNoise.cpp
        Array_2D.hpp
        SimplexNoise.hpp
)