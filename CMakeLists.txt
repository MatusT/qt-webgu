cmake_minimum_required(VERSION 3.24)

project(qt-webgpu LANGUAGES CXX C)

include(utils.cmake)

add_subdirectory(webgpu)

find_package(Qt6 6.6 REQUIRED COMPONENTS Core Widgets)

add_library(wgpu-native SHARED IMPORTED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

add_executable(main main.cpp WebGPUWindow.cpp)

target_link_libraries(main PRIVATE Qt6::Core Qt6::Widgets webgpu)

set_target_properties(main PROPERTIES
    CXX_STANDARD 20
    COMPILE_WARNING_AS_ERROR ON
)

target_treat_all_warnings_as_errors(main)
target_copy_webgpu_binaries(main)