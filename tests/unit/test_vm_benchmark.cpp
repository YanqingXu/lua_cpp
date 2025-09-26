/**
 * @file test_vm_benchmark.cpp
 * @brief 虚拟机性能基准测试
 * @description 验证虚拟机执行性能和与参考项目的对比
 * @date 2025-09-26
 * @version T025 - Virtual Machine Performance Validation
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <chrono>
#include <random>

#include "vm/virtual_machine.h"
#include "compiler/bytecode.h"
#include "core/lua_common.h"

using namespace lua_cpp;

/* ========================================================================== */
/* 性能基准测试辅助函数 */
/* ========================================================================== */

/**
 * @brief 创建简单的算术测试程序
 */
std::unique_ptr<Proto> CreateArithmeticTestProgram() {
    auto proto = std::make_unique<Proto>("arithmetic_test");
    
    // 添加常量
    proto->AddConstant(LuaValue(1.0));
    proto->AddConstant(LuaValue(2.0));
    proto->AddConstant(LuaValue(3.0));
    
    // 生成指令: for i=1,1000 do result = result + i*2 end
    // 简化版本：执行一系列算术操作
    
    // LOADK R0, K0 (1.0)
    proto->AddInstruction(CreateABx(OpCode::LOADK, 0, 0), 1);
    
    // LOADK R1, K1 (2.0) 
    proto->AddInstruction(CreateABx(OpCode::LOADK, 1, 1), 2);
    
    // 循环1000次
    for (int i = 0; i < 1000; ++i) {
        // MUL R2, R0, R1
        proto->AddInstruction(CreateABC(OpCode::MUL, 2, 0, 1), 3);
        
        // ADD R0, R0, R2
        proto->AddInstruction(CreateABC(OpCode::ADD, 0, 0, 2), 4);
    }
    
    // RETURN R0, 2
    proto->AddInstruction(CreateABC(OpCode::RETURN, 0, 2, 0), 5);
    
    proto->SetParameterCount(0);
    proto->SetMaxStackSize(10);
    
    return proto;
}

/**
 * @brief 创建表操作测试程序
 */
std::unique_ptr<Proto> CreateTableTestProgram() {
    auto proto = std::make_unique<Proto>("table_test");
    
    // 添加常量
    proto->AddConstant(LuaValue("key"));
    proto->AddConstant(LuaValue(42.0));
    
    // NEWTABLE R0, 5, 5 (创建表)
    proto->AddInstruction(CreateABC(OpCode::NEWTABLE, 0, 5, 5), 1);
    
    // 执行100次表操作
    for (int i = 0; i < 100; ++i) {
        // LOADK R1, K0 ("key")
        proto->AddInstruction(CreateABx(OpCode::LOADK, 1, 0), 2);
        
        // LOADK R2, K1 (42.0)
        proto->AddInstruction(CreateABx(OpCode::LOADK, 2, 1), 3);
        
        // SETTABLE R0, R1, R2
        proto->AddInstruction(CreateABC(OpCode::SETTABLE, 0, 1, 2), 4);
        
        // GETTABLE R3, R0, R1
        proto->AddInstruction(CreateABC(OpCode::GETTABLE, 3, 0, 1), 5);
    }
    
    // RETURN R3, 2
    proto->AddInstruction(CreateABC(OpCode::RETURN, 3, 2, 0), 6);
    
    proto->SetParameterCount(0);
    proto->SetMaxStackSize(10);
    
    return proto;
}

/**
 * @brief 创建函数调用测试程序
 */
std::unique_ptr<Proto> CreateCallTestProgram() {
    auto proto = std::make_unique<Proto>("call_test");
    
    // 添加常量
    proto->AddConstant(LuaValue(1.0));
    proto->AddConstant(LuaValue(100.0));
    
    // 简单的递归计数器（模拟）
    // LOADK R0, K0 (1.0)
    proto->AddInstruction(CreateABx(OpCode::LOADK, 0, 0), 1);
    
    // LOADK R1, K1 (100.0)
    proto->AddInstruction(CreateABx(OpCode::LOADK, 1, 1), 2);
    
    // 循环模拟函数调用开销
    for (int i = 0; i < 50; ++i) {
        // ADD R0, R0, R1
        proto->AddInstruction(CreateABC(OpCode::ADD, 0, 0, 1), 3);
    }
    
    // RETURN R0, 2
    proto->AddInstruction(CreateABC(OpCode::RETURN, 0, 2, 0), 4);
    
    proto->SetParameterCount(0);
    proto->SetMaxStackSize(10);
    
    return proto;
}

/* ========================================================================== */
/* 基本性能基准测试 */
/* ========================================================================== */

TEST_CASE("VM Benchmark - 虚拟机基础操作性能", "[vm][benchmark][basic]") {
    BENCHMARK("VM 创建和初始化") {
        return CreateStandardVM();
    };
    
    BENCHMARK("堆栈操作 - Push/Pop 1000次") {
        auto vm = CreateStandardVM();
        
        return [&vm]() {
            for (int i = 0; i < 1000; ++i) {
                vm->Push(LuaValue(static_cast<double>(i)));
            }
            for (int i = 0; i < 1000; ++i) {
                vm->Pop();
            }
        }();
    };
    
    BENCHMARK("寄存器操作 - 设置/获取 1000次") {
        auto vm = CreateStandardVM();
        auto proto = std::make_unique<Proto>("benchmark");
        vm->PushCallFrame(proto.get(), 0, 0, 0);
        
        return [&vm]() {
            for (int i = 0; i < 1000; ++i) {
                vm->SetRegister(i % 10, LuaValue(static_cast<double>(i)));
                vm->GetRegister(i % 10);
            }
        }();
    };
}

TEST_CASE("VM Benchmark - 指令执行性能", "[vm][benchmark][instruction]") {
    BENCHMARK("算术指令执行") {
        auto vm = CreateStandardVM();
        auto proto = CreateArithmeticTestProgram();
        
        return vm->ExecuteProgram(proto.get());
    };
    
    BENCHMARK("单条MOVE指令") {
        auto vm = CreateStandardVM();
        auto proto = std::make_unique<Proto>("move_test");
        vm->PushCallFrame(proto.get(), 0, 0, 0);
        
        vm->SetRegister(1, LuaValue(42.0));
        Instruction move_inst = CreateABC(OpCode::MOVE, 0, 1, 0);
        
        return [&vm, move_inst]() {
            vm->ExecuteInstruction(move_inst);
        }();
    };
    
    BENCHMARK("单条ADD指令") {
        auto vm = CreateStandardVM();
        auto proto = std::make_unique<Proto>("add_test");
        vm->PushCallFrame(proto.get(), 0, 0, 0);
        
        vm->SetRegister(1, LuaValue(10.0));
        vm->SetRegister(2, LuaValue(5.0));
        Instruction add_inst = CreateABC(OpCode::ADD, 0, 1, 2);
        
        return [&vm, add_inst]() {
            vm->ExecuteInstruction(add_inst);
        }();
    };
}

TEST_CASE("VM Benchmark - 表操作性能", "[vm][benchmark][table]") {
    BENCHMARK("表操作程序") {
        auto vm = CreateStandardVM();
        auto proto = CreateTableTestProgram();
        
        return vm->ExecuteProgram(proto.get());
    };
    
    BENCHMARK("单次表创建") {
        auto vm = CreateStandardVM();
        auto proto = std::make_unique<Proto>("newtable_test");
        vm->PushCallFrame(proto.get(), 0, 0, 0);
        
        Instruction newtable_inst = CreateABC(OpCode::NEWTABLE, 0, 3, 3);
        
        return [&vm, newtable_inst]() {
            vm->ExecuteInstruction(newtable_inst);
        }();
    };
}

/* ========================================================================== */
/* 内存和GC性能测试 */
/* ========================================================================== */

TEST_CASE("VM Benchmark - 内存管理性能", "[vm][benchmark][memory]") {
    BENCHMARK("大量LuaValue创建") {
        std::vector<LuaValue> values;
        values.reserve(10000);
        
        return [&values]() {
            for (int i = 0; i < 10000; ++i) {
                values.emplace_back(static_cast<double>(i));
            }
        }();
    };
    
    BENCHMARK("表对象创建和销毁") {
        std::vector<std::shared_ptr<LuaTable>> tables;
        tables.reserve(1000);
        
        return [&tables]() {
            for (int i = 0; i < 1000; ++i) {
                tables.push_back(std::make_shared<LuaTable>());
                tables.back()->Set(LuaValue("key" + std::to_string(i)), 
                                  LuaValue(static_cast<double>(i)));
            }
        }();
    };
}

/* ========================================================================== */
/* 综合性能测试 */
/* ========================================================================== */

TEST_CASE("VM Benchmark - 综合程序性能", "[vm][benchmark][comprehensive]") {
    BENCHMARK("复杂算术计算程序") {
        auto vm = CreateStandardVM();
        auto proto = CreateArithmeticTestProgram();
        
        return vm->ExecuteProgram(proto.get());
    };
    
    BENCHMARK("混合操作程序") {
        auto vm = CreateStandardVM();
        auto proto = std::make_unique<Proto>("mixed_test");
        
        // 添加常量
        proto->AddConstant(LuaValue(1.0));
        proto->AddConstant(LuaValue(2.0));
        proto->AddConstant(LuaValue("test_key"));
        
        // 混合指令：算术 + 表操作 + 字符串操作
        
        // NEWTABLE R0
        proto->AddInstruction(CreateABC(OpCode::NEWTABLE, 0, 2, 2), 1);
        
        // LOADK R1, K0 (1.0)
        proto->AddInstruction(CreateABx(OpCode::LOADK, 1, 0), 2);
        
        // LOADK R2, K1 (2.0)
        proto->AddInstruction(CreateABx(OpCode::LOADK, 2, 1), 3);
        
        // 循环100次混合操作
        for (int i = 0; i < 100; ++i) {
            // ADD R3, R1, R2
            proto->AddInstruction(CreateABC(OpCode::ADD, 3, 1, 2), 4);
            
            // SETTABLE R0, R1, R3
            proto->AddInstruction(CreateABC(OpCode::SETTABLE, 0, 1, 3), 5);
            
            // GETTABLE R4, R0, R1
            proto->AddInstruction(CreateABC(OpCode::GETTABLE, 4, 0, 1), 6);
            
            // ADD R1, R1, R2 (increment counter)
            proto->AddInstruction(CreateABC(OpCode::ADD, 1, 1, 2), 7);
        }
        
        // RETURN R4, 2
        proto->AddInstruction(CreateABC(OpCode::RETURN, 4, 2, 0), 8);
        
        proto->SetParameterCount(0);
        proto->SetMaxStackSize(20);
        
        return vm->ExecuteProgram(proto.get());
    };
}

/* ========================================================================== */
/* 性能验证和统计 */
/* ========================================================================== */

TEST_CASE("VM Performance Validation - 执行统计验证", "[vm][performance][validation]") {
    SECTION("执行统计准确性") {
        auto vm = CreateStandardVM();
        auto proto = CreateArithmeticTestProgram();
        
        vm->ResetStatistics();
        
        auto start = std::chrono::high_resolution_clock::now();
        auto results = vm->ExecuteProgram(proto.get());
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration<double>(end - start);
        auto stats = vm->GetExecutionStatistics();
        
        // 验证统计信息
        REQUIRE(stats.total_instructions > 0);
        REQUIRE(stats.execution_time > 0);
        REQUIRE(duration.count() >= stats.execution_time * 0.9); // 允许10%误差
        
        // 验证结果
        REQUIRE(!results.empty());
        REQUIRE(results[0].IsNumber());
        
        std::cout << "执行统计:" << std::endl;
        std::cout << "总指令数: " << stats.total_instructions << std::endl;
        std::cout << "执行时间: " << stats.execution_time * 1000 << " ms" << std::endl;
        std::cout << "指令/秒: " << static_cast<Size>(stats.total_instructions / stats.execution_time) << std::endl;
    }
    
    SECTION("内存使用效率") {
        auto vm = CreateStandardVM();
        
        Size initial_memory = vm->GetMemoryUsage();
        
        // 执行内存密集操作
        for (int i = 0; i < 1000; ++i) {
            vm->Push(LuaValue(static_cast<double>(i)));
        }
        
        Size peak_memory = vm->GetMemoryUsage();
        auto peak_stack = vm->GetExecutionStatistics().peak_stack_usage;
        
        REQUIRE(peak_memory > initial_memory);
        REQUIRE(peak_stack >= 1000);
        
        std::cout << "内存使用:" << std::endl;
        std::cout << "初始内存: " << initial_memory << " bytes" << std::endl;
        std::cout << "峰值内存: " << peak_memory << " bytes" << std::endl;
        std::cout << "峰值堆栈: " << peak_stack << " slots" << std::endl;
    }
}

/* ========================================================================== */
/* 性能对比基准 */
/* ========================================================================== */

TEST_CASE("VM Performance Comparison - 性能目标验证", "[vm][performance][target]") {
    SECTION("指令执行速度目标") {
        auto vm = CreateStandardVM();
        auto proto = CreateArithmeticTestProgram();
        
        vm->ResetStatistics();
        
        auto start = std::chrono::high_resolution_clock::now();
        vm->ExecuteProgram(proto.get());
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration<double>(end - start);
        auto stats = vm->GetExecutionStatistics();
        
        double instructions_per_second = stats.total_instructions / duration.count();
        
        // 性能目标：至少100万条指令/秒
        REQUIRE(instructions_per_second >= 1000000.0);
        
        std::cout << "性能指标:" << std::endl;
        std::cout << "指令执行速度: " << static_cast<Size>(instructions_per_second) << " 指令/秒" << std::endl;
        
        INFO("达到性能目标：>= 1M instructions/second");
    }
    
    SECTION("内存效率目标") {
        auto vm = CreateStandardVM();
        auto proto = CreateTableTestProgram();
        
        Size before_memory = vm->GetMemoryUsage();
        vm->ExecuteProgram(proto.get());
        Size after_memory = vm->GetMemoryUsage();
        
        Size memory_used = after_memory - before_memory;
        
        // 内存使用应该合理（不超过100KB用于简单程序）
        REQUIRE(memory_used < 100 * 1024);
        
        std::cout << "内存效率:" << std::endl;
        std::cout << "程序内存开销: " << memory_used << " bytes" << std::endl;
        
        INFO("达到内存效率目标：< 100KB overhead");
    }
}