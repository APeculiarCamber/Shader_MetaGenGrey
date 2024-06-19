cmake_minimum_required(VERSION 3.27)
project(Shader_MetaGen)

set(CMAKE_CXX_STANDARD 23)
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")

add_compile_definitions(SHADER_DIR="${CMAKE_SOURCE_DIR}/Shaders/")

add_executable(Shader_MetaGen # vulkan_utils.h
        SPIRV-Reflect/spirv_reflect.c
        SPIRV-Reflect/spirv_reflect.h
        main.cpp
        TypeDescriptors.h
)
