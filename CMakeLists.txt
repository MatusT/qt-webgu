cmake_minimum_required(VERSION 3.24)

project(qt-webgpu LANGUAGES CXX)

find_package(Qt6 6.6 REQUIRED COMPONENTS Core Widgets)

add_library(wgpu-native SHARED IMPORTED)
set_property(TARGET wgpu-native PROPERTY IMPORTED_LOCATION "${PROJECT_SOURCE_DIR}/dependencies/wgpu-native/wgpu_native.dll")
set_property(TARGET wgpu-native PROPERTY IMPORTED_IMPLIB "${PROJECT_SOURCE_DIR}/dependencies/wgpu-native/wgpu_native.dll.lib")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

add_executable(main main.cpp WebGPUWindow.cpp)

target_include_directories(main PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/wgpu-native/")

target_link_libraries(main PRIVATE Qt6::Core Qt6::Widgets wgpu-native)

set_target_properties(main PROPERTIES
    CXX_STANDARD 20
    COMPILE_WARNING_AS_ERROR ON
)

if (MSVC)
    target_compile_options(main PRIVATE /W4)
else()
    target_compile_options(main PRIVATE -Wall -Wextra -pedantic)
endif()
