cmake_minimum_required(VERSION 3.24)

project(qt-webgpu LANGUAGES CXX C)

include(utils.cmake)

add_subdirectory(webgpu)

if (EMSCRIPTEN)
    add_executable(renderer Renderer.hpp Renderer.cpp)

    set_target_properties(renderer PROPERTIES
        CXX_STANDARD 20
    )

    target_link_libraries(renderer PRIVATE webgpu)

    # set_target_properties(${EXECUTABLE_NAME} PROPERTIES LINK_FLAGS "-s ENVIRONMENT=web -s SINGLE_FILE=1 -s MODULARIZE -s 'EXPORT_NAME=DummyCppLibrary' --bind")
    set_target_properties(renderer PROPERTIES LINK_FLAGS "-s ENVIRONMENT=web -s \"EXPORT_NAME=Renderer\" -s EXPORT_ES6=1 -s MODULARIZE -s EXPORTED_RUNTIME_METHODS=[WebGPU]")
    # Add Emscripten-specific link options
    target_link_options(renderer PRIVATE
        -sALLOW_MEMORY_GROWTH
        -sUSE_WEBGPU # Handle WebGPU symbols
        -sASYNCIFY # Required by WebGPU-C++
        -lembind
    )

    # target_treat_all_warnings_as_errors(renderer)
    target_copy_webgpu_binaries(renderer)
else()
    find_package(Qt6 6.6 REQUIRED COMPONENTS Core Widgets)

    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)

    add_executable(main main.cpp implementations.cpp WebGPUWindow.hpp WebGPUWindow.cpp Renderer.hpp Renderer.cpp)

    target_link_libraries(main PRIVATE Qt6::Core Qt6::Widgets webgpu)

    set_target_properties(main PROPERTIES
        CXX_STANDARD 20
        COMPILE_WARNING_AS_ERROR ON
    )

    target_treat_all_warnings_as_errors(main)
    target_copy_webgpu_binaries(main)             
endif() 
