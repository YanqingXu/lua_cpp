# 测试CMake配置

# T026集成测试
add_executable(t026_integration_test
    integration/test_t026_integration.cpp
)

target_link_libraries(t026_integration_test
    lua_cpp_lib
    Threads::Threads
)

target_include_directories(t026_integration_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# 传统集成测试
add_executable(vm_integration_test
    integration/vm_test.cpp
)

target_link_libraries(vm_integration_test
    lua_cpp_lib
    Threads::Threads
)

target_include_directories(vm_integration_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# GC集成测试
add_executable(gc_integration_test
    integration/gc_test.cpp
)

target_link_libraries(gc_integration_test
    lua_cpp_lib
    Threads::Threads
)

target_include_directories(gc_integration_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# 单元测试（如果存在Catch2）
if(TARGET Catch2::Catch2WithMain)
    # T026单元测试
    add_executable(t026_unit_tests
        unit/test_call_stack_advanced_unit.cpp
        unit/test_upvalue_manager_unit.cpp
    )
    
    target_link_libraries(t026_unit_tests
        lua_cpp_lib
        Catch2::Catch2WithMain
        Threads::Threads
    )
    
    target_include_directories(t026_unit_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/src
    )
    
    # 收集所有其他单元测试文件
    file(GLOB_RECURSE OTHER_UNIT_TEST_SOURCES
        "unit/test_*.cpp"
    )
    
    # 排除T026测试文件
    list(FILTER OTHER_UNIT_TEST_SOURCES EXCLUDE REGEX "test_call_stack_advanced_unit\\.cpp")
    list(FILTER OTHER_UNIT_TEST_SOURCES EXCLUDE REGEX "test_upvalue_manager_unit\\.cpp")
    
    if(OTHER_UNIT_TEST_SOURCES)
        add_executable(unit_tests ${OTHER_UNIT_TEST_SOURCES})
        
        target_link_libraries(unit_tests
            lua_cpp_lib
            Catch2::Catch2WithMain
            Threads::Threads
        )
        
        target_include_directories(unit_tests PRIVATE
            ${CMAKE_SOURCE_DIR}/src
        )
        
        # 添加到CTest
        include(Catch)
        catch_discover_tests(unit_tests)
    endif()
    
    # T026单元测试也添加到CTest
    catch_discover_tests(t026_unit_tests)
endif()

# T026合约测试
add_executable(t026_contract_tests
    contract/test_call_stack_advanced_contract.cpp
    contract/test_upvalue_manager_contract.cpp
    contract/test_coroutine_support_contract.cpp
)

target_link_libraries(t026_contract_tests
    lua_cpp_lib
    Threads::Threads
)

target_include_directories(t026_contract_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# 如果有Catch2，链接它
if(TARGET Catch2::Catch2WithMain)
    target_link_libraries(t026_contract_tests Catch2::Catch2WithMain)
endif()

# 其他合约测试
file(GLOB_RECURSE OTHER_CONTRACT_TEST_SOURCES
    "contract/test_*.cpp"
)

# 排除T026合约测试文件
list(FILTER OTHER_CONTRACT_TEST_SOURCES EXCLUDE REGEX "test_call_stack_advanced_contract\\.cpp")
list(FILTER OTHER_CONTRACT_TEST_SOURCES EXCLUDE REGEX "test_upvalue_manager_contract\\.cpp")
list(FILTER OTHER_CONTRACT_TEST_SOURCES EXCLUDE REGEX "test_coroutine_support_contract\\.cpp")

if(OTHER_CONTRACT_TEST_SOURCES)
    add_executable(contract_tests ${OTHER_CONTRACT_TEST_SOURCES})
    
    target_link_libraries(contract_tests
        lua_cpp_lib
        Threads::Threads
    )
    
    target_include_directories(contract_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/src
    )
    
    if(TARGET Catch2::Catch2WithMain)
        target_link_libraries(contract_tests Catch2::Catch2WithMain)
    endif()
endif()

# 性能基准测试（如果存在Google Benchmark）
find_package(benchmark QUIET)
if(TARGET benchmark::benchmark)
    add_executable(t026_benchmark_tests
        benchmark/test_t026_benchmark.cpp
    )
    
    target_link_libraries(t026_benchmark_tests
        lua_cpp_lib
        benchmark::benchmark
        Threads::Threads
    )
    
    target_include_directories(t026_benchmark_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/src
    )
endif()

# 注册测试
add_test(NAME t026_integration_test COMMAND t026_integration_test)
add_test(NAME t026_contract_tests COMMAND t026_contract_tests)

if(TARGET t026_unit_tests)
    add_test(NAME t026_unit_tests COMMAND t026_unit_tests)
endif()

if(TARGET t026_benchmark_tests)
    add_test(NAME t026_benchmark_tests COMMAND t026_benchmark_tests --benchmark_min_time=0.1)
endif()

add_test(NAME vm_integration_test COMMAND vm_integration_test)
add_test(NAME gc_integration_test COMMAND gc_integration_test)

if(TARGET unit_tests)
    add_test(NAME unit_tests COMMAND unit_tests)
endif()

if(TARGET contract_tests)
    add_test(NAME contract_tests COMMAND contract_tests)
endif()

# 设置测试属性
set_tests_properties(t026_integration_test PROPERTIES
    LABELS "integration;t026"
    TIMEOUT 60
)

set_tests_properties(t026_contract_tests PROPERTIES
    LABELS "contract;t026"
    TIMEOUT 30
)

if(TARGET t026_unit_tests)
    set_tests_properties(t026_unit_tests PROPERTIES
        LABELS "unit;t026"
        TIMEOUT 20
    )
endif()

if(TARGET t026_benchmark_tests)
    set_tests_properties(t026_benchmark_tests PROPERTIES
        LABELS "benchmark;t026;performance"
        TIMEOUT 300
    )
endif()

set_tests_properties(vm_integration_test PROPERTIES
    LABELS "integration"
    TIMEOUT 30
)

set_tests_properties(gc_integration_test PROPERTIES
    LABELS "integration;gc"
    TIMEOUT 60
)

if(TARGET unit_tests)
    set_tests_properties(unit_tests PROPERTIES
        LABELS "unit"
        TIMEOUT 10
    )
endif()

if(TARGET contract_tests)
    set_tests_properties(contract_tests PROPERTIES
        LABELS "contract"
        TIMEOUT 15
    )
endif()