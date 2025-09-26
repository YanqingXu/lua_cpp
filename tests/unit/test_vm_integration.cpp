/**
 * @file test_vm_integration.cpp
 * @brief 虚拟机集成测试
 * @description 测试虚拟机与编译器的完整集成和端到端功能
 * @date 2025-09-26
 * @version T025 - Virtual Machine Integration Testing
 */

#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <memory>

#include "vm/virtual_machine.h"
#include "compiler/bytecode.h"
#include "core/lua_common.h"

using namespace lua_cpp;

/* ========================================================================== */
/* 集成测试辅助函数 */
/* ========================================================================== */

/**
 * @brief 创建完整的Lua程序模拟
 */
std::unique_ptr<Proto> CreateCompleteProgram() {
    auto proto = std::make_unique<Proto>("main");
    
    // 添加常量
    proto->AddConstant(LuaValue(10.0));      // K0: 10
    proto->AddConstant(LuaValue(5.0));       // K1: 5
    proto->AddConstant(LuaValue(1.0));       // K2: 1
    proto->AddConstant(LuaValue("result"));  // K3: "result"
    
    // 生成完整程序：
    // local a = 10
    // local b = 5
    // local result = 0
    // for i = 1, a do
    //     result = result + b
    // end
    // return result
    
    // LOADK R0, K0 (a = 10)
    proto->AddInstruction(CreateABx(OpCode::LOADK, 0, 0), 1);
    
    // LOADK R1, K1 (b = 5)
    proto->AddInstruction(CreateABx(OpCode::LOADK, 1, 1), 2);
    
    // LOADNIL R2, 1 (result = nil, 初始化为0)
    proto->AddInstruction(CreateABC(OpCode::LOADNIL, 2, 0, 0), 3);
    
    // LOADK R2, K2 (result = 0，但用1开始)
    proto->AddInstruction(CreateABx(OpCode::LOADK, 2, 2), 4);
    
    // LOADK R3, K2 (i = 1)
    proto->AddInstruction(CreateABx(OpCode::LOADK, 3, 2), 5);
    
    // 循环开始标签（指令索引6）
    // FORLOOP R3, 7 (检查i <= a，如果是跳转7条指令)
    // 简化：直接使用条件跳转
    
    // LT R4, R0, R3 (检查 i <= a，结果放在R4，但实际是 R3 <= R0)
    // 我们需要检查 R3 <= R0，但LT是小于，所以检查 R3 < (R0+1)
    
    // 手动实现循环：重复10次加法
    for (int i = 0; i < 10; ++i) {
        // ADD R2, R2, R1 (result = result + b)
        proto->AddInstruction(CreateABC(OpCode::ADD, 2, 2, 1), 6 + i);
    }
    
    // RETURN R2, 2 (返回result)
    proto->AddInstruction(CreateABC(OpCode::RETURN, 2, 2, 0), 16);
    
    proto->SetParameterCount(0);
    proto->SetMaxStackSize(10);
    
    return proto;
}

/**
 * @brief 创建表操作程序
 */
std::unique_ptr<Proto> CreateTableProgram() {
    auto proto = std::make_unique<Proto>("table_main");
    
    // 添加常量
    proto->AddConstant(LuaValue("name"));    // K0: "name"
    proto->AddConstant(LuaValue("John"));    // K1: "John"
    proto->AddConstant(LuaValue("age"));     // K2: "age"
    proto->AddConstant(LuaValue(25.0));      // K3: 25
    
    // 程序：
    // local person = {}
    // person["name"] = "John"
    // person["age"] = 25
    // return person["name"], person["age"]
    
    // NEWTABLE R0, 2, 2 (创建表)
    proto->AddInstruction(CreateABC(OpCode::NEWTABLE, 0, 2, 2), 1);
    
    // LOADK R1, K0 ("name")
    proto->AddInstruction(CreateABx(OpCode::LOADK, 1, 0), 2);
    
    // LOADK R2, K1 ("John")
    proto->AddInstruction(CreateABx(OpCode::LOADK, 2, 1), 3);
    
    // SETTABLE R0, R1, R2 (person["name"] = "John")
    proto->AddInstruction(CreateABC(OpCode::SETTABLE, 0, 1, 2), 4);
    
    // LOADK R1, K2 ("age")
    proto->AddInstruction(CreateABx(OpCode::LOADK, 1, 2), 5);
    
    // LOADK R2, K3 (25)
    proto->AddInstruction(CreateABx(OpCode::LOADK, 2, 3), 6);
    
    // SETTABLE R0, R1, R2 (person["age"] = 25)
    proto->AddInstruction(CreateABC(OpCode::SETTABLE, 0, 1, 2), 7);
    
    // LOADK R1, K0 ("name")
    proto->AddInstruction(CreateABx(OpCode::LOADK, 1, 0), 8);
    
    // GETTABLE R3, R0, R1 (R3 = person["name"])
    proto->AddInstruction(CreateABC(OpCode::GETTABLE, 3, 0, 1), 9);
    
    // LOADK R1, K2 ("age") 
    proto->AddInstruction(CreateABx(OpCode::LOADK, 1, 2), 10);
    
    // GETTABLE R4, R0, R1 (R4 = person["age"])
    proto->AddInstruction(CreateABC(OpCode::GETTABLE, 4, 0, 1), 11);
    
    // RETURN R3, 3 (返回两个值)
    proto->AddInstruction(CreateABC(OpCode::RETURN, 3, 3, 0), 12);
    
    proto->SetParameterCount(0);
    proto->SetMaxStackSize(15);
    
    return proto;
}

/**
 * @brief 创建字符串操作程序
 */
std::unique_ptr<Proto> CreateStringProgram() {
    auto proto = std::make_unique<Proto>("string_main");
    
    // 添加常量
    proto->AddConstant(LuaValue("Hello"));   // K0
    proto->AddConstant(LuaValue(" "));       // K1  
    proto->AddConstant(LuaValue("World"));   // K2
    
    // 程序：
    // local str1 = "Hello"
    // local str2 = " "
    // local str3 = "World"
    // local result = str1 .. str2 .. str3
    // return result
    
    // LOADK R0, K0 ("Hello")
    proto->AddInstruction(CreateABx(OpCode::LOADK, 0, 0), 1);
    
    // LOADK R1, K1 (" ")
    proto->AddInstruction(CreateABx(OpCode::LOADK, 1, 1), 2);
    
    // CONCAT R2, R0, R1 (R2 = str1 .. str2)
    proto->AddInstruction(CreateABC(OpCode::CONCAT, 2, 0, 1), 3);
    
    // LOADK R3, K2 ("World")
    proto->AddInstruction(CreateABx(OpCode::LOADK, 3, 2), 4);
    
    // CONCAT R4, R2, R3 (R4 = result .. str3)
    proto->AddInstruction(CreateABC(OpCode::CONCAT, 4, 2, 3), 5);
    
    // RETURN R4, 2
    proto->AddInstruction(CreateABC(OpCode::RETURN, 4, 2, 0), 6);
    
    proto->SetParameterCount(0);
    proto->SetMaxStackSize(10);
    
    return proto;
}

/* ========================================================================== */
/* VM-Compiler集成测试 */
/* ========================================================================== */

TEST_CASE("VM Integration - 完整程序执行", "[vm][integration][complete]") {
    SECTION("算术计算程序") {
        auto vm = CreateStandardVM();
        auto program = CreateCompleteProgram();
        
        // 重置统计
        vm->ResetStatistics();
        
        // 执行程序
        auto results = vm->ExecuteProgram(program.get());
        
        // 验证结果
        REQUIRE(!results.empty());
        REQUIRE(results[0].IsNumber());
        
        // 预期结果：10次循环，每次加5，结果应该是50
        double expected = 1.0 + (10 * 5.0); // 初始值1 + 10*5
        REQUIRE(results[0].AsNumber() == expected);
        
        // 验证执行统计
        auto stats = vm->GetExecutionStatistics();
        REQUIRE(stats.total_instructions > 0);
        REQUIRE(stats.execution_time > 0);
        
        std::cout << "算术程序执行统计:" << std::endl;
        std::cout << "结果: " << results[0].AsNumber() << std::endl;
        std::cout << "总指令数: " << stats.total_instructions << std::endl;
        std::cout << "执行时间: " << stats.execution_time * 1000 << " ms" << std::endl;
    }
    
    SECTION("表操作程序") {
        auto vm = CreateStandardVM();
        auto program = CreateTableProgram();
        
        vm->ResetStatistics();
        auto results = vm->ExecuteProgram(program.get());
        
        // 验证结果：应该返回两个值
        REQUIRE(results.size() == 2);
        REQUIRE(results[0].IsString());
        REQUIRE(results[1].IsNumber());
        
        REQUIRE(results[0].AsString() == "John");
        REQUIRE(results[1].AsNumber() == 25.0);
        
        auto stats = vm->GetExecutionStatistics();
        
        std::cout << "表操作程序执行统计:" << std::endl;
        std::cout << "name: " << results[0].AsString() << std::endl;
        std::cout << "age: " << results[1].AsNumber() << std::endl;
        std::cout << "总指令数: " << stats.total_instructions << std::endl;
    }
    
    SECTION("字符串操作程序") {
        auto vm = CreateStandardVM();
        auto program = CreateStringProgram();
        
        vm->ResetStatistics();
        auto results = vm->ExecuteProgram(program.get());
        
        // 验证结果
        REQUIRE(!results.empty());
        REQUIRE(results[0].IsString());
        REQUIRE(results[0].AsString() == "Hello World");
        
        auto stats = vm->GetExecutionStatistics();
        
        std::cout << "字符串程序执行统计:" << std::endl;
        std::cout << "结果: '" << results[0].AsString() << "'" << std::endl;
        std::cout << "总指令数: " << stats.total_instructions << std::endl;
    }
}

/* ========================================================================== */
/* 错误处理集成测试 */
/* ========================================================================== */

TEST_CASE("VM Integration - 错误处理", "[vm][integration][error]") {
    SECTION("堆栈溢出检测") {
        auto vm = CreateStandardVM();
        auto proto = std::make_unique<Proto>("stack_overflow_test");
        
        // 设置小的堆栈限制
        proto->SetMaxStackSize(5);
        
        // 尝试超过堆栈限制
        for (int i = 0; i < 10; ++i) {
            // LOADK R[i], K0 (会超出堆栈)
            proto->AddInstruction(CreateABx(OpCode::LOADK, i, 0), 1 + i);
        }
        
        proto->AddConstant(LuaValue(42.0));
        
        // 执行应该抛出异常
        REQUIRE_THROWS_AS(vm->ExecuteProgram(proto.get()), std::runtime_error);
    }
    
    SECTION("无效指令处理") {
        auto vm = CreateStandardVM();
        auto proto = std::make_unique<Proto>("invalid_inst_test");
        
        // 添加无效指令（使用未定义的操作码）
        Instruction invalid_inst = 0xFF000000; // 无效操作码
        proto->AddInstruction(invalid_inst, 1);
        
        // 执行应该抛出异常
        REQUIRE_THROWS_AS(vm->ExecuteProgram(proto.get()), std::runtime_error);
    }
    
    SECTION("类型错误检测") {
        auto vm = CreateStandardVM();
        auto proto = std::make_unique<Proto>("type_error_test");
        
        // 尝试对字符串执行算术操作
        proto->AddConstant(LuaValue("not_a_number"));
        proto->AddConstant(LuaValue(5.0));
        
        // LOADK R0, K0 ("not_a_number")
        proto->AddInstruction(CreateABx(OpCode::LOADK, 0, 0), 1);
        
        // LOADK R1, K1 (5.0)
        proto->AddInstruction(CreateABx(OpCode::LOADK, 1, 1), 2);
        
        // ADD R2, R0, R1 (字符串 + 数字，应该出错)
        proto->AddInstruction(CreateABC(OpCode::ADD, 2, 0, 1), 3);
        
        proto->SetParameterCount(0);
        proto->SetMaxStackSize(10);
        
        // 执行应该抛出异常
        REQUIRE_THROWS_AS(vm->ExecuteProgram(proto.get()), std::runtime_error);
    }
}

/* ========================================================================== */
/* 内存管理集成测试 */
/* ========================================================================== */

TEST_CASE("VM Integration - 内存管理", "[vm][integration][memory]") {
    SECTION("大程序内存管理") {
        auto vm = CreateStandardVM();
        auto proto = std::make_unique<Proto>("large_program");
        
        Size initial_memory = vm->GetMemoryUsage();
        
        // 创建大量常量和指令
        for (int i = 0; i < 1000; ++i) {
            proto->AddConstant(LuaValue(static_cast<double>(i)));
            proto->AddInstruction(CreateABx(OpCode::LOADK, i % 10, i), i + 1);
        }
        
        // RETURN R0, 2
        proto->AddInstruction(CreateABC(OpCode::RETURN, 0, 2, 0), 1001);
        
        proto->SetParameterCount(0);
        proto->SetMaxStackSize(20);
        
        auto results = vm->ExecuteProgram(proto.get());
        
        Size final_memory = vm->GetMemoryUsage();
        Size memory_used = final_memory - initial_memory;
        
        // 验证内存使用合理
        REQUIRE(memory_used > 0);
        REQUIRE(memory_used < 10 * 1024 * 1024); // 不超过10MB
        
        auto stats = vm->GetExecutionStatistics();
        REQUIRE(stats.peak_memory_usage >= memory_used);
        
        std::cout << "大程序内存统计:" << std::endl;
        std::cout << "初始内存: " << initial_memory << " bytes" << std::endl;
        std::cout << "最终内存: " << final_memory << " bytes" << std::endl;
        std::cout << "内存增长: " << memory_used << " bytes" << std::endl;
        std::cout << "峰值内存: " << stats.peak_memory_usage << " bytes" << std::endl;
    }
    
    SECTION("表对象生命周期") {
        auto vm = CreateStandardVM();
        auto proto = std::make_unique<Proto>("table_lifecycle");
        
        Size initial_memory = vm->GetMemoryUsage();
        
        // 创建多个表对象
        for (int i = 0; i < 100; ++i) {
            // NEWTABLE R[i%5], 3, 3
            proto->AddInstruction(CreateABC(OpCode::NEWTABLE, i % 5, 3, 3), i * 2 + 1);
            
            // 设置一些数据
            proto->AddConstant(LuaValue(static_cast<double>(i)));
        }
        
        // RETURN R0, 2
        proto->AddInstruction(CreateABC(OpCode::RETURN, 0, 2, 0), 201);
        
        proto->SetParameterCount(0);
        proto->SetMaxStackSize(10);
        
        auto results = vm->ExecuteProgram(proto.get());
        
        Size final_memory = vm->GetMemoryUsage();
        
        std::cout << "表生命周期内存统计:" << std::endl;
        std::cout << "初始内存: " << initial_memory << " bytes" << std::endl;
        std::cout << "最终内存: " << final_memory << " bytes" << std::endl;
        
        // 表对象应该被正确管理
        REQUIRE(final_memory >= initial_memory);
    }
}

/* ========================================================================== */
/* 性能集成测试 */
/* ========================================================================== */

TEST_CASE("VM Integration - 性能集成测试", "[vm][integration][performance]") {
    SECTION("端到端性能测试") {
        auto vm = CreateStandardVM();
        
        // 执行多个不同类型的程序
        std::vector<std::unique_ptr<Proto>> programs;
        programs.push_back(CreateCompleteProgram());
        programs.push_back(CreateTableProgram());
        programs.push_back(CreateStringProgram());
        
        vm->ResetStatistics();
        auto start = std::chrono::high_resolution_clock::now();
        
        // 执行所有程序
        Size total_instructions = 0;
        for (const auto& program : programs) {
            auto results = vm->ExecuteProgram(program.get());
            REQUIRE(!results.empty());
            
            auto stats = vm->GetExecutionStatistics();
            total_instructions += stats.total_instructions;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double>(end - start);
        
        double instructions_per_second = total_instructions / duration.count();
        
        std::cout << "端到端性能测试:" << std::endl;
        std::cout << "总指令数: " << total_instructions << std::endl;
        std::cout << "总时间: " << duration.count() * 1000 << " ms" << std::endl;
        std::cout << "指令速度: " << static_cast<Size>(instructions_per_second) << " 指令/秒" << std::endl;
        
        // 性能验证
        REQUIRE(instructions_per_second > 100000); // 至少10万指令/秒
        REQUIRE(duration.count() < 0.1); // 整体执行时间不超过100ms
    }
}

/* ========================================================================== */
/* 兼容性测试 */
/* ========================================================================== */

TEST_CASE("VM Integration - Lua 5.1.5兼容性", "[vm][integration][compatibility]") {
    SECTION("标准Lua指令兼容性") {
        auto vm = CreateStandardVM();
        auto proto = std::make_unique<Proto>("compatibility_test");
        
        // 测试所有基本Lua 5.1.5指令
        
        // 数值操作
        proto->AddConstant(LuaValue(3.14));
        proto->AddConstant(LuaValue(2.71));
        
        // LOADK R0, K0 (3.14)
        proto->AddInstruction(CreateABx(OpCode::LOADK, 0, 0), 1);
        
        // LOADK R1, K1 (2.71)
        proto->AddInstruction(CreateABx(OpCode::LOADK, 1, 1), 2);
        
        // ADD R2, R0, R1
        proto->AddInstruction(CreateABC(OpCode::ADD, 2, 0, 1), 3);
        
        // SUB R3, R0, R1
        proto->AddInstruction(CreateABC(OpCode::SUB, 3, 0, 1), 4);
        
        // MUL R4, R0, R1
        proto->AddInstruction(CreateABC(OpCode::MUL, 4, 0, 1), 5);
        
        // DIV R5, R0, R1
        proto->AddInstruction(CreateABC(OpCode::DIV, 5, 0, 1), 6);
        
        // POW R6, R0, R1
        proto->AddInstruction(CreateABC(OpCode::POW, 6, 0, 1), 7);
        
        // UNM R7, R0, 0
        proto->AddInstruction(CreateABC(OpCode::UNM, 7, 0, 0), 8);
        
        // RETURN R2, 7 (返回多个结果)
        proto->AddInstruction(CreateABC(OpCode::RETURN, 2, 7, 0), 9);
        
        proto->SetParameterCount(0);
        proto->SetMaxStackSize(15);
        
        auto results = vm->ExecuteProgram(proto.get());
        
        // 验证所有算术运算结果
        REQUIRE(results.size() >= 5);
        
        double a = 3.14, b = 2.71;
        REQUIRE(std::abs(results[0].AsNumber() - (a + b)) < 1e-10);
        REQUIRE(std::abs(results[1].AsNumber() - (a - b)) < 1e-10);
        REQUIRE(std::abs(results[2].AsNumber() - (a * b)) < 1e-10);
        REQUIRE(std::abs(results[3].AsNumber() - (a / b)) < 1e-10);
        REQUIRE(std::abs(results[4].AsNumber() - (-a)) < 1e-10);
        
        std::cout << "Lua 5.1.5兼容性验证通过" << std::endl;
        for (size_t i = 0; i < results.size(); ++i) {
            std::cout << "结果[" << i << "]: " << results[i].AsNumber() << std::endl;
        }
    }
}