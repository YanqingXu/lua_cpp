cmake_minimum_required(VERSION 3.16)

# 项目信息
project(lua_cpp 
    VERSION 1.0.0
    DESCRIPTION "Modern C++ Lua 5.1.5 Interpreter"
    LANGUAGES CXX
)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 启用C++20特性（如果编译器支持）
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17 /Zc:__cplusplus /utf-8")
endif()

# 编译器警告设置
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -pedantic")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -fsanitize=address,undefined")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -flto")
    
    # Lua特定优化
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -ffast-math")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -funroll-loops")
    
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /utf-8")
    # 禁用C4819编码警告
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4819")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /Ob2 /DNDEBUG /GL")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
    set(CMAKE_STATIC_LINKER_FLAGS_RELEASE "${CMAKE_STATIC_LINKER_FLAGS_RELEASE} /LTCG")
endif()

# 输出目录设置
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 选项
option(BUILD_TESTING "Build tests" ON)
option(BUILD_BENCHMARKS "Build benchmarks" ON)
option(ENABLE_COVERAGE "Enable code coverage" OFF)
option(ENABLE_PROFILING "Enable profiling" OFF)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(LUA_CPP_USE_LONGJMP "Use longjmp for error handling" OFF)
option(LUA_CPP_ENABLE_DEBUG_OUTPUT "Enable debug output" OFF)
option(LUA_CPP_STRICT_COMPATIBILITY "Enable strict Lua 5.1.5 compatibility" ON)

# 查找依赖
find_package(Threads REQUIRED)

# 测试框架
if(BUILD_TESTING)
    enable_testing()
    
    # 尝试查找Catch2
    find_package(Catch2 3 QUIET)
    if(NOT Catch2_FOUND)
        # 如果找不到，使用FetchContent下载
        include(FetchContent)
        FetchContent_Declare(
            Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG v3.4.0
        )
        FetchContent_MakeAvailable(Catch2)
    endif()
endif()

# 性能测试框架
if(BUILD_BENCHMARKS)
    find_package(benchmark QUIET)
    if(NOT benchmark_FOUND)
        include(FetchContent)
        FetchContent_Declare(
            googlebenchmark
            GIT_REPOSITORY https://github.com/google/benchmark.git
            GIT_TAG v1.8.3
        )
        FetchContent_MakeAvailable(googlebenchmark)
    endif()
endif()

# 代码覆盖率
if(ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()

# 包含目录
include_directories(src)

# 核心库
add_subdirectory(src)

# 测试
if(BUILD_TESTING)
    add_subdirectory(tests)
    
    # T026测试
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/T026_tests")
        add_subdirectory(tests/T026_tests)
    endif()
endif()

# 性能测试
if(BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

# 安装规则
install(TARGETS lua_cpp
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# 安装核心库
install(TARGETS lua_cpp_lib
    EXPORT lua_cpp_targets
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include/lua_cpp
)

# 安装头文件（保持目录结构）
install(DIRECTORY src/
    DESTINATION include/lua_cpp
    FILES_MATCHING PATTERN "*.h"
    PATTERN "*.hpp"
)

# 安装导出配置
install(EXPORT lua_cpp_targets
    FILE lua_cpp_targets.cmake
    NAMESPACE lua_cpp::
    DESTINATION lib/cmake/lua_cpp
)

# 创建配置文件
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/lua_cpp_config_version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/lua_cpp_config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/lua_cpp_config.cmake"
    INSTALL_DESTINATION lib/cmake/lua_cpp
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/lua_cpp_config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/lua_cpp_config_version.cmake"
    DESTINATION lib/cmake/lua_cpp
)

# 文档
find_package(Doxygen QUIET)
if(Doxygen_FOUND)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in
        ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        @ONLY
    )
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
endif()

# CPack包配置
set(CPACK_PACKAGE_NAME "lua_cpp")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_CONTACT "lua_cpp@example.com")

include(CPack)