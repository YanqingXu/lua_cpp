# 测试CMake配置

# 集成测试
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
    # 收集所有单元测试文件
    file(GLOB_RECURSE UNIT_TEST_SOURCES
        "unit/*.cpp"
    )
    
    if(UNIT_TEST_SOURCES)
        add_executable(unit_tests ${UNIT_TEST_SOURCES})
        
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
endif()

# 合约测试
file(GLOB_RECURSE CONTRACT_TEST_SOURCES
    "contract/*.cpp"
)

if(CONTRACT_TEST_SOURCES)
    add_executable(contract_tests ${CONTRACT_TEST_SOURCES})
    
    target_link_libraries(contract_tests
        lua_cpp_lib
        Threads::Threads
    )
    
    target_include_directories(contract_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/src
    )
endif()

# 注册测试
add_test(NAME vm_integration_test COMMAND vm_integration_test)
add_test(NAME gc_integration_test COMMAND gc_integration_test)

if(TARGET unit_tests)
    add_test(NAME unit_tests COMMAND unit_tests)
endif()

if(TARGET contract_tests)
    add_test(NAME contract_tests COMMAND contract_tests)
endif()

# 设置测试属性
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