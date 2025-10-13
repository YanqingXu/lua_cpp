# T028 Phase 3 - 协程库最小化测试的独立CMake配置

cmake_minimum_required(VERSION 3.16)
project(T028_Coroutine_Minimal_Test CXX)

# C++20标准（协程支持）
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MSVC编译器选项
if(MSVC)
    add_compile_options(/std:c++20 /Zc:__cplusplus /utf-8 /await:strict)
    message(STATUS "Enabled C++20 coroutines for MSVC")
else()
    add_compile_options(-std=c++20 -fcoroutines)
    message(STATUS "Enabled C++20 coroutines for GCC/Clang")
endif()

# 创建测试可执行文件
add_executable(test_coroutine_minimal
    test_coroutine_lib_minimal.cpp
)

# 输出信息
message(STATUS "T028 Minimal Coroutine Test Configuration:")
message(STATUS "  - C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  - Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "  - Build Type: ${CMAKE_BUILD_TYPE}")

# 设置输出目录
set_target_properties(test_coroutine_minimal PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

message(STATUS "Target 'test_coroutine_minimal' configured successfully")
