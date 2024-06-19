cmake_minimum_required(VERSION 3.27)
project(Shader_MetaGen)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")

add_compile_definitions(SHADER_DIR="${CMAKE_SOURCE_DIR}/")

add_executable(GreyPixelEngine # vulkan_utils.h
        main.cpp
)
