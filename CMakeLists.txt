cmake_minimum_required(VERSION 3.12)
project(LuaCpp VERSION 0.1.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 添加源文件
file(GLOB_RECURSE LIB_SOURCES
    "src/common/*.cpp"
    "src/compiler/*.cpp"
    "src/gc/*.cpp"
    "src/object/*.cpp"
    "src/vm/*.cpp"
    "src/lib/*.cpp"
    "src/api/*.cpp"
)

# 添加头文件
file(GLOB_RECURSE LIB_HEADERS
    "include/lua/*.hpp"
    "src/common/*.hpp"
    "src/compiler/*.hpp"
    "src/gc/*.hpp"
    "src/object/*.hpp"
    "src/vm/*.hpp"
    "src/lib/*.hpp"
    "src/api/*.hpp"
)

# 创建静态库
add_library(luacpp STATIC ${LIB_SOURCES} ${LIB_HEADERS})

# 创建Lua解释器可执行文件
add_executable(lua src/lua.cpp)
target_link_libraries(lua luacpp)

# 安装目标
install(TARGETS luacpp lua
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/lua
    DESTINATION include
)

# 添加测试目录
add_subdirectory(test)
