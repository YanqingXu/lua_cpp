# 核心库CMake配置

# 收集所有源文件
file(GLOB_RECURSE LUA_CPP_SOURCES
    "core/*.cpp"
    "lexer/*.cpp"
    "parser/*.cpp"
    "compiler/*.cpp"
    "vm/*.cpp"
    "memory/*.cpp"
    "types/*.cpp"
    "stdlib/*.cpp"
    "api/*.cpp"
)

# 收集所有头文件（包含新创建的接口文件）
file(GLOB_RECURSE LUA_CPP_HEADERS
    "core/*.h"
    "lexer/*.h"
    "parser/*.h"
    "compiler/*.h"
    "vm/*.h"
    "memory/*.h"
    "types/*.h"
    "stdlib/*.h"
    "api/*.h"
)

# 显式列出重要的接口头文件以确保CMake识别
set(LUA_CPP_INTERFACE_HEADERS
    # 核心接口
    "core/common.h"
    "core/lua_value.h"
    "core/error.h"
    
    # 解析器接口
    "parser/ast.h"
    "parser/parser.h"
    
    # 编译器接口
    "compiler/bytecode.h"
    "compiler/compiler.h"
    
    # 虚拟机接口
    "vm/stack.h"
    "vm/call_frame.h"
    "vm/virtual_machine.h"
    
    # 内存管理接口
    "memory/garbage_collector.h"
    
    # API接口
    "api/lua_api.h"
    "api/luaaux.h"
)

# 创建核心库
add_library(lua_cpp_lib STATIC 
    ${LUA_CPP_SOURCES} 
    ${LUA_CPP_HEADERS}
    ${LUA_CPP_INTERFACE_HEADERS}
)

# 设置库属性
set_target_properties(lua_cpp_lib PROPERTIES
    OUTPUT_NAME "lua_cpp"
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    PUBLIC_HEADER "${LUA_CPP_INTERFACE_HEADERS}"
)

# 链接线程库
target_link_libraries(lua_cpp_lib Threads::Threads)

# 包含目录
target_include_directories(lua_cpp_lib
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include/lua_cpp>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# 编译器特定设置
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(lua_cpp_lib PRIVATE
        -fPIC
        -fvisibility=hidden
    )
    
    # 启用计算goto优化（如果支持）
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag("-fno-gcse" COMPILER_SUPPORTS_NO_GCSE)
    if(COMPILER_SUPPORTS_NO_GCSE)
        target_compile_options(lua_cpp_lib PRIVATE -fno-gcse)
    endif()
endif()

# 预处理器定义
target_compile_definitions(lua_cpp_lib PRIVATE
    LUA_CPP_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    LUA_CPP_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    LUA_CPP_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    
    # Lua版本兼容性
    LUA_VERSION_NUM=501
    LUA_VERSION_MAJOR=5
    LUA_VERSION_MINOR=1
    LUA_VERSION_MICRO=5
    LUA_VERSION="Lua 5.1.5"
    LUA_RELEASE="Lua 5.1.5"
    LUA_COPYRIGHT="Copyright (C) 1994-2012 Lua.org, PUC-Rio"
    
    # 编译器特性
    $<$<CXX_COMPILER_ID:GNU>:LUA_CPP_GCC>
    $<$<CXX_COMPILER_ID:Clang>:LUA_CPP_CLANG>
    $<$<CXX_COMPILER_ID:MSVC>:LUA_CPP_MSVC>
)

# 公共定义（用户可见）
target_compile_definitions(lua_cpp_lib PUBLIC
    LUA_CPP_LIB
    $<$<CONFIG:Debug>:LUA_CPP_DEBUG>
)

# 创建可执行文件
add_executable(lua_cpp cli/lua_cli.cpp)

# 链接核心库
target_link_libraries(lua_cpp lua_cpp_lib)

# 设置可执行文件属性
set_target_properties(lua_cpp PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)