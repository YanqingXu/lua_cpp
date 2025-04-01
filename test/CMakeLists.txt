cmake_minimum_required(VERSION 3.12)

# 包含目录
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# 设置子目录
add_subdirectory(unit)
add_subdirectory(integration)

# 添加单元测试可执行文件
file(GLOB_RECURSE UNIT_TEST_SOURCES "unit/*.cpp")
add_executable(unit_tests ${UNIT_TEST_SOURCES})
target_link_libraries(unit_tests luacpp)

# 添加集成测试可执行文件
file(GLOB_RECURSE INTEGRATION_TEST_SOURCES "integration/*.cpp")
add_executable(integration_tests ${INTEGRATION_TEST_SOURCES})
target_link_libraries(integration_tests luacpp)
