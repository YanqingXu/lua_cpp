/**
 * @file test_vm_unit.cpp
 * @brief 虚拟机单元测试
 * @description 基于契约测试的虚拟机功能验证和单元测试
 * @date 2025-09-26
 * @version T025 - Virtual Machine Executor Implementation
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "vm/virtual_machine.h"
#include "vm/call_frame.h"
#include "compiler/bytecode.h"
#include "core/lua_common.h"
#include "core/lua_errors.h"
#include "types/lua_table.h"

using namespace lua_cpp;
using Catch::Approx;

/* ========================================================================== */
/* VM基础架构单元测试 */
/* ========================================================================== */

TEST_CASE("VM Unit - 虚拟机初始化和配置", "[vm][unit][basic]") {
    SECTION("默认配置初始化") {
        auto vm = CreateStandardVM();
        
        REQUIRE(vm != nullptr);
        REQUIRE(vm->GetExecutionState() == ExecutionState::Ready);
        REQUIRE(vm->GetStackTop() == 0);
        REQUIRE(vm->GetCallFrameCount() == 0);
        REQUIRE(vm->GetInstructionPointer() == 0);
    }

    SECTION("自定义配置初始化") {
        VMConfig config;
        config.initial_stack_size = 512;
        config.max_stack_size = 2048;
        config.max_call_depth = 200;
        config.enable_debug_info = true;
        
        VirtualMachine vm(config);
        
        REQUIRE(vm.GetMaxStackSize() == 2048);
        REQUIRE(vm.IsDebugEnabled() == true);
        REQUIRE(vm.GetExecutionState() == ExecutionState::Ready);
    }

    SECTION("VM工厂函数") {
        auto debug_vm = CreateDebugVM();
        REQUIRE(debug_vm->IsDebugEnabled() == true);
        REQUIRE(debug_vm->IsProfilingEnabled() == true);
        
        auto embedded_vm = CreateEmbeddedVM();
        REQUIRE(embedded_vm->GetMaxStackSize() <= 1024);
    }
}

/* ========================================================================== */
/* 堆栈操作单元测试 */
/* ========================================================================== */

TEST_CASE("VM Unit - 堆栈操作", "[vm][unit][stack]") {
    auto vm = CreateStandardVM();
    
    SECTION("基本堆栈Push/Pop操作") {
        LuaValue val1(42.0);
        LuaValue val2("test");
        LuaValue val3(true);
        
        vm->Push(val1);
        vm->Push(val2);
        vm->Push(val3);
        
        REQUIRE(vm->GetStackTop() == 3);
        
        REQUIRE(vm->Top().IsBoolean());
        REQUIRE(vm->Top().GetBoolean() == true);
        
        LuaValue popped = vm->Pop();
        REQUIRE(popped.IsBoolean());
        REQUIRE(vm->GetStackTop() == 2);
        
        REQUIRE(vm->GetStack(0).IsNumber());
        REQUIRE(vm->GetStack(0).GetNumber() == Approx(42.0));
    }

    SECTION("堆栈索引访问") {
        for (int i = 0; i < 5; ++i) {
            vm->Push(LuaValue(static_cast<double>(i)));
        }
        
        REQUIRE(vm->GetStack(2).GetNumber() == Approx(2.0));
        
        vm->SetStack(2, LuaValue(99.0));
        REQUIRE(vm->GetStack(2).GetNumber() == Approx(99.0));
    }

    SECTION("堆栈边界检查") {
        // 测试访问越界索引
        REQUIRE_THROWS_AS(vm->GetStack(1000), std::exception);
        REQUIRE_THROWS_AS(vm->SetStack(1000, LuaValue()), std::exception);
    }
}

/* ========================================================================== */
/* 寄存器操作单元测试 */
/* ========================================================================== */

TEST_CASE("VM Unit - 寄存器操作", "[vm][unit][register]") {
    auto vm = CreateStandardVM();
    
    // 创建简单的函数原型和调用帧
    auto proto = std::make_unique<Proto>("test");
    vm->PushCallFrame(proto.get(), 0, 0, 0);
    
    SECTION("寄存器读写") {
        vm->SetRegister(0, LuaValue(123.0));
        vm->SetRegister(1, LuaValue("hello"));
        vm->SetRegister(2, LuaValue(false));
        
        REQUIRE(vm->GetRegister(0).GetNumber() == Approx(123.0));
        REQUIRE(vm->GetRegister(1).GetString() == "hello");
        REQUIRE(vm->GetRegister(2).GetBoolean() == false);
    }

    SECTION("寄存器边界检查") {
        // 测试非法寄存器索引
        REQUIRE_THROWS_AS(vm->SetRegister(256, LuaValue()), VMExecutionError);
        REQUIRE_THROWS_AS(vm->GetRegister(256), VMExecutionError);
    }

    SECTION("RK值处理") {
        // 添加常量到函数原型
        proto->AddConstant(LuaValue(456.0));
        proto->AddConstant(LuaValue("world"));
        
        vm->SetRegister(0, LuaValue(789.0));
        
        // 测试寄存器值 (RK without BITRK)
        LuaValue reg_val = vm->GetRK(0);
        REQUIRE(reg_val.GetNumber() == Approx(789.0));
        
        // 测试常量值 (RK with BITRK)
        LuaValue const_val = vm->GetRK(ConstantIndexToRK(0));
        REQUIRE(const_val.GetNumber() == Approx(456.0));
    }
}

/* ========================================================================== */
/* 指令执行单元测试 */
/* ========================================================================== */

TEST_CASE("VM Unit - 数据移动指令", "[vm][unit][instruction][move]") {
    auto vm = CreateStandardVM();
    auto proto = std::make_unique<Proto>("test");
    proto->AddConstant(LuaValue("constant_string"));
    proto->AddConstant(LuaValue(3.14));
    
    vm->PushCallFrame(proto.get(), 0, 0, 0);
    
    SECTION("MOVE指令") {
        vm->SetRegister(1, LuaValue(42.0));
        
        Instruction move_inst = CreateABC(OpCode::MOVE, 0, 1, 0);
        vm->ExecuteInstruction(move_inst);
        
        REQUIRE(vm->GetRegister(0).GetNumber() == Approx(42.0));
    }

    SECTION("LOADK指令") {
        Instruction loadk_inst = CreateABx(OpCode::LOADK, 0, 0);
        vm->ExecuteInstruction(loadk_inst);
        
        REQUIRE(vm->GetRegister(0).GetString() == "constant_string");
    }

    SECTION("LOADBOOL指令") {
        Size initial_pc = vm->GetInstructionPointer();
        
        Instruction loadbool_inst = CreateABC(OpCode::LOADBOOL, 0, 1, 1);
        vm->ExecuteInstruction(loadbool_inst);
        
        REQUIRE(vm->GetRegister(0).GetBoolean() == true);
        REQUIRE(vm->GetInstructionPointer() == initial_pc + 2); // 跳过下一条指令
    }

    SECTION("LOADNIL指令") {
        // 先设置非nil值
        vm->SetRegister(0, LuaValue(1.0));
        vm->SetRegister(1, LuaValue(2.0));
        vm->SetRegister(2, LuaValue(3.0));
        
        Instruction loadnil_inst = CreateABC(OpCode::LOADNIL, 0, 2, 0);
        vm->ExecuteInstruction(loadnil_inst);
        
        REQUIRE(vm->GetRegister(0).IsNil());
        REQUIRE(vm->GetRegister(1).IsNil());
        REQUIRE(vm->GetRegister(2).IsNil());
    }
}

TEST_CASE("VM Unit - 算术指令", "[vm][unit][instruction][arithmetic]") {
    auto vm = CreateStandardVM();
    auto proto = std::make_unique<Proto>("test");
    vm->PushCallFrame(proto.get(), 0, 0, 0);
    
    SECTION("基本算术操作") {
        vm->SetRegister(1, LuaValue(10.0));
        vm->SetRegister(2, LuaValue(3.0));
        
        // ADD
        Instruction add_inst = CreateABC(OpCode::ADD, 0, 1, 2);
        vm->ExecuteInstruction(add_inst);
        REQUIRE(vm->GetRegister(0).GetNumber() == Approx(13.0));
        
        // SUB
        Instruction sub_inst = CreateABC(OpCode::SUB, 0, 1, 2);
        vm->ExecuteInstruction(sub_inst);
        REQUIRE(vm->GetRegister(0).GetNumber() == Approx(7.0));
        
        // MUL
        Instruction mul_inst = CreateABC(OpCode::MUL, 0, 1, 2);
        vm->ExecuteInstruction(mul_inst);
        REQUIRE(vm->GetRegister(0).GetNumber() == Approx(30.0));
        
        // DIV
        Instruction div_inst = CreateABC(OpCode::DIV, 0, 1, 2);
        vm->ExecuteInstruction(div_inst);
        REQUIRE(vm->GetRegister(0).GetNumber() == Approx(10.0/3.0));
    }

    SECTION("一元算术操作") {
        vm->SetRegister(1, LuaValue(42.0));
        
        // UNM (unary minus)
        Instruction unm_inst = CreateABC(OpCode::UNM, 0, 1, 0);
        vm->ExecuteInstruction(unm_inst);
        REQUIRE(vm->GetRegister(0).GetNumber() == Approx(-42.0));
        
        // NOT
        vm->SetRegister(1, LuaValue(false));
        Instruction not_inst = CreateABC(OpCode::NOT, 0, 1, 0);
        vm->ExecuteInstruction(not_inst);
        REQUIRE(vm->GetRegister(0).GetBoolean() == true);
    }

    SECTION("除零错误处理") {
        vm->SetRegister(1, LuaValue(10.0));
        vm->SetRegister(2, LuaValue(0.0));
        
        Instruction div_inst = CreateABC(OpCode::DIV, 0, 1, 2);
        REQUIRE_THROWS_AS(vm->ExecuteInstruction(div_inst), VMExecutionError);
    }

    SECTION("类型错误处理") {
        vm->SetRegister(1, LuaValue("not_a_number"));
        vm->SetRegister(2, LuaValue(5.0));
        
        Instruction add_inst = CreateABC(OpCode::ADD, 0, 1, 2);
        REQUIRE_THROWS_AS(vm->ExecuteInstruction(add_inst), TypeError);
    }
}

/* ========================================================================== */
/* 表操作单元测试 */
/* ========================================================================== */

TEST_CASE("VM Unit - 表操作指令", "[vm][unit][instruction][table]") {
    auto vm = CreateStandardVM();
    auto proto = std::make_unique<Proto>("test");
    vm->PushCallFrame(proto.get(), 0, 0, 0);
    
    SECTION("NEWTABLE指令") {
        Instruction newtable_inst = CreateABC(OpCode::NEWTABLE, 0, 2, 1);
        vm->ExecuteInstruction(newtable_inst);
        
        LuaValue result = vm->GetRegister(0);
        REQUIRE(result.IsTable());
        
        auto table = result.GetTable();
        REQUIRE(table != nullptr);
    }

    SECTION("表索引操作") {
        // 创建表并设置值
        auto table = std::make_shared<LuaTable>();
        table->Set(LuaValue("key"), LuaValue(123.0));
        
        vm->SetRegister(1, LuaValue(table));
        vm->SetRegister(2, LuaValue("key"));
        
        // GETTABLE
        Instruction gettable_inst = CreateABC(OpCode::GETTABLE, 0, 1, 2);
        vm->ExecuteInstruction(gettable_inst);
        
        REQUIRE(vm->GetRegister(0).GetNumber() == Approx(123.0));
        
        // SETTABLE
        vm->SetRegister(3, LuaValue(456.0));
        Instruction settable_inst = CreateABC(OpCode::SETTABLE, 1, 2, 3);
        vm->ExecuteInstruction(settable_inst);
        
        LuaValue stored = table->Get(LuaValue("key"));
        REQUIRE(stored.GetNumber() == Approx(456.0));
    }

    SECTION("SELF指令") {
        auto table = std::make_shared<LuaTable>();
        table->Set(LuaValue("method"), LuaValue("method_func"));
        
        vm->SetRegister(1, LuaValue(table));
        vm->SetRegister(2, LuaValue("method"));
        
        Instruction self_inst = CreateABC(OpCode::SELF, 0, 1, 2);
        vm->ExecuteInstruction(self_inst);
        
        // R(0) 应该包含方法
        REQUIRE(vm->GetRegister(0).GetString() == "method_func");
        
        // R(1) 应该包含表对象 (self)
        REQUIRE(vm->GetRegister(1).IsTable());
    }
}

/* ========================================================================== */
/* 比较和跳转指令单元测试 */
/* ========================================================================== */

TEST_CASE("VM Unit - 比较和跳转指令", "[vm][unit][instruction][comparison]") {
    auto vm = CreateStandardVM();
    auto proto = std::make_unique<Proto>("test");
    vm->PushCallFrame(proto.get(), 0, 0, 0);
    
    SECTION("EQ指令") {
        vm->SetRegister(1, LuaValue(42.0));
        vm->SetRegister(2, LuaValue(42.0));
        
        Size initial_pc = vm->GetInstructionPointer();
        
        // 相等时跳转
        Instruction eq_inst = CreateABC(OpCode::EQ, 1, 1, 2);
        vm->ExecuteInstruction(eq_inst);
        
        REQUIRE(vm->GetInstructionPointer() == initial_pc + 2);
    }

    SECTION("LT指令") {
        vm->SetRegister(1, LuaValue(5.0));
        vm->SetRegister(2, LuaValue(10.0));
        
        Size initial_pc = vm->GetInstructionPointer();
        
        Instruction lt_inst = CreateABC(OpCode::LT, 1, 1, 2);
        vm->ExecuteInstruction(lt_inst);
        
        REQUIRE(vm->GetInstructionPointer() == initial_pc + 2);
    }

    SECTION("JMP指令") {
        Size initial_pc = vm->GetInstructionPointer();
        int jump_offset = 5;
        
        Instruction jmp_inst = CreateAsBx(OpCode::JMP, 0, jump_offset);
        vm->ExecuteInstruction(jmp_inst);
        
        REQUIRE(vm->GetInstructionPointer() == initial_pc + jump_offset);
    }

    SECTION("TEST指令") {
        vm->SetRegister(1, LuaValue(true));
        
        Size initial_pc = vm->GetInstructionPointer();
        
        // 测试真值，C=0 (测试假)
        Instruction test_inst = CreateABC(OpCode::TEST, 1, 0, 0);
        vm->ExecuteInstruction(test_inst);
        
        // true != false，所以应该跳转
        REQUIRE(vm->GetInstructionPointer() == initial_pc + 2);
    }
}

/* ========================================================================== */
/* 字符串操作单元测试 */
/* ========================================================================== */

TEST_CASE("VM Unit - 字符串操作", "[vm][unit][instruction][string]") {
    auto vm = CreateStandardVM();
    auto proto = std::make_unique<Proto>("test");
    vm->PushCallFrame(proto.get(), 0, 0, 0);
    
    SECTION("LEN指令 - 字符串长度") {
        vm->SetRegister(1, LuaValue("hello"));
        
        Instruction len_inst = CreateABC(OpCode::LEN, 0, 1, 0);
        vm->ExecuteInstruction(len_inst);
        
        REQUIRE(vm->GetRegister(0).GetNumber() == Approx(5.0));
    }

    SECTION("CONCAT指令 - 字符串连接") {
        vm->SetRegister(1, LuaValue("Hello"));
        vm->SetRegister(2, LuaValue(" "));
        vm->SetRegister(3, LuaValue("World"));
        
        Instruction concat_inst = CreateABC(OpCode::CONCAT, 0, 1, 3);
        vm->ExecuteInstruction(concat_inst);
        
        REQUIRE(vm->GetRegister(0).GetString() == "Hello World");
    }

    SECTION("CONCAT指令 - 数字转字符串") {
        vm->SetRegister(1, LuaValue("Number: "));
        vm->SetRegister(2, LuaValue(42.0));
        
        Instruction concat_inst = CreateABC(OpCode::CONCAT, 0, 1, 2);
        vm->ExecuteInstruction(concat_inst);
        
        std::string result = vm->GetRegister(0).GetString();
        REQUIRE(result.find("Number: ") == 0);
        REQUIRE(result.find("42") != std::string::npos);
    }
}

/* ========================================================================== */
/* 循环指令单元测试 */
/* ========================================================================== */

TEST_CASE("VM Unit - 循环指令", "[vm][unit][instruction][loop]") {
    auto vm = CreateStandardVM();
    auto proto = std::make_unique<Proto>("test");
    vm->PushCallFrame(proto.get(), 0, 0, 0);
    
    SECTION("FORPREP指令") {
        vm->SetRegister(0, LuaValue(10.0));  // init
        vm->SetRegister(1, LuaValue(1.0));   // limit  
        vm->SetRegister(2, LuaValue(-2.0));  // step
        
        Size initial_pc = vm->GetInstructionPointer();
        int jump = 3;
        
        Instruction forprep_inst = CreateAsBx(OpCode::FORPREP, 0, jump);
        vm->ExecuteInstruction(forprep_inst);
        
        // init = init - step = 10 - (-2) = 12
        REQUIRE(vm->GetRegister(0).GetNumber() == Approx(12.0));
        REQUIRE(vm->GetInstructionPointer() == initial_pc + jump);
    }

    SECTION("FORLOOP指令 - 继续循环") {
        vm->SetRegister(0, LuaValue(1.0));   // init
        vm->SetRegister(1, LuaValue(5.0));   // limit
        vm->SetRegister(2, LuaValue(1.0));   // step
        
        Size initial_pc = vm->GetInstructionPointer();
        int jump = -2;
        
        Instruction forloop_inst = CreateAsBx(OpCode::FORLOOP, 0, jump);
        vm->ExecuteInstruction(forloop_inst);
        
        // init = init + step = 1 + 1 = 2
        REQUIRE(vm->GetRegister(0).GetNumber() == Approx(2.0));
        // 2 <= 5，所以继续循环
        REQUIRE(vm->GetInstructionPointer() == initial_pc + jump);
        // 循环变量
        REQUIRE(vm->GetRegister(3).GetNumber() == Approx(2.0));
    }

    SECTION("FORLOOP指令 - 结束循环") {
        vm->SetRegister(0, LuaValue(5.0));   // init
        vm->SetRegister(1, LuaValue(5.0));   // limit
        vm->SetRegister(2, LuaValue(1.0));   // step
        
        Size initial_pc = vm->GetInstructionPointer();
        
        Instruction forloop_inst = CreateAsBx(OpCode::FORLOOP, 0, -2);
        vm->ExecuteInstruction(forloop_inst);
        
        // init = 5 + 1 = 6, 6 > 5，所以不跳转
        REQUIRE(vm->GetRegister(0).GetNumber() == Approx(6.0));
        REQUIRE(vm->GetInstructionPointer() == initial_pc + 1);
    }
}

/* ========================================================================== */
/* 错误处理和边界情况单元测试 */
/* ========================================================================== */

TEST_CASE("VM Unit - 错误处理", "[vm][unit][error]") {
    auto vm = CreateStandardVM();
    
    SECTION("无调用帧执行指令") {
        Instruction move_inst = CreateABC(OpCode::MOVE, 0, 1, 0);
        
        REQUIRE_THROWS_AS(vm->ExecuteInstruction(move_inst), VMExecutionError);
    }

    SECTION("无效操作码") {
        auto proto = std::make_unique<Proto>("test");
        vm->PushCallFrame(proto.get(), 0, 0, 0);
        
        // 创建无效指令
        Instruction invalid_inst = 0xFFFFFFFF;
        
        REQUIRE_THROWS_AS(vm->ExecuteInstruction(invalid_inst), InvalidInstructionError);
    }

    SECTION("堆栈溢出") {
        VMConfig config;
        config.max_stack_size = 10;
        VirtualMachine small_vm(config);
        
        auto proto = std::make_unique<Proto>("test");
        small_vm.PushCallFrame(proto.get(), 0, 0, 0);
        
        REQUIRE_THROWS_AS([&]() {
            for (int i = 0; i < 20; ++i) {
                small_vm.SetRegister(i, LuaValue(i));
            }
        }(), VMExecutionError);
    }

    SECTION("指令限制") {
        VMConfig config;
        config.enable_instruction_limit = true;
        config.instruction_limit = 5;
        VirtualMachine limited_vm(config);
        
        auto proto = std::make_unique<Proto>("test");
        limited_vm.PushCallFrame(proto.get(), 0, 0, 0);
        
        Instruction nop_inst = CreateABC(OpCode::LOADNIL, 0, 0, 0);
        
        // 执行5条指令应该正常
        for (int i = 0; i < 5; ++i) {
            limited_vm.ExecuteInstruction(nop_inst);
        }
        
        // 第6条指令应该抛出异常
        REQUIRE_THROWS_AS(limited_vm.ExecuteInstruction(nop_inst), VMExecutionError);
    }
}

/* ========================================================================== */
/* 统计和诊断单元测试 */
/* ========================================================================== */

TEST_CASE("VM Unit - 统计和诊断", "[vm][unit][statistics]") {
    auto vm = CreateStandardVM();
    auto proto = std::make_unique<Proto>("test");
    vm->PushCallFrame(proto.get(), 0, 0, 0);
    
    SECTION("指令执行统计") {
        vm->ResetStatistics();
        
        Instruction move_inst = CreateABC(OpCode::MOVE, 0, 1, 0);
        Instruction add_inst = CreateABC(OpCode::ADD, 0, 1, 2);
        
        vm->ExecuteInstruction(move_inst);
        vm->ExecuteInstruction(add_inst);
        
        auto stats = vm->GetExecutionStatistics();
        REQUIRE(stats.total_instructions == 2);
        REQUIRE(stats.instruction_counts[static_cast<int>(OpCode::MOVE)] == 1);
        REQUIRE(stats.instruction_counts[static_cast<int>(OpCode::ADD)] == 1);
    }

    SECTION("调试信息") {
        auto debug_info = vm->GetCurrentDebugInfo();
        REQUIRE(debug_info.current_function == proto.get());
        REQUIRE(debug_info.instruction_pointer == 0);
    }

    SECTION("内存使用量") {
        Size initial_memory = vm->GetMemoryUsage();
        
        // 推入一些值
        for (int i = 0; i < 10; ++i) {
            vm->Push(LuaValue(i));
        }
        
        Size after_memory = vm->GetMemoryUsage();
        REQUIRE(after_memory > initial_memory);
    }
}