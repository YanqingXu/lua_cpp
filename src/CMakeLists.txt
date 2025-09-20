# 核心库CMake配置

# 收集所有源文件
file(GLOB_RECURSE LUA_CPP_SOURCES
    "core/*.cpp"
    "lexer/*.cpp"
    "parser/*.cpp"
    "compiler/*.cpp"
    "vm/*.cpp"
    "gc/*.cpp"
    "types/*.cpp"
    "stdlib/*.cpp"
    "api/*.cpp"
)

file(GLOB_RECURSE LUA_CPP_HEADERS
    "core/*.h"
    "lexer/*.h"
    "parser/*.h"
    "compiler/*.h"
    "vm/*.h"
    "gc/*.h"
    "types/*.h"
    "stdlib/*.h"
    "api/*.h"
)

# 创建核心库
add_library(lua_cpp_lib STATIC ${LUA_CPP_SOURCES} ${LUA_CPP_HEADERS})

# 设置库属性
set_target_properties(lua_cpp_lib PROPERTIES
    OUTPUT_NAME "lua_cpp"
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
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
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(lua_cpp_lib PRIVATE LUA_CPP_DEBUG)
endif()

# 创建可执行文件
add_executable(lua_cpp cli/lua_cli.cpp)

# 链接核心库
target_link_libraries(lua_cpp lua_cpp_lib)

# 设置可执行文件属性
set_target_properties(lua_cpp PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)