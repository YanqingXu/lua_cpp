/**
 * @file test_vm_contract.cpp
 * @brief VM（虚拟机）契约测试
 * @description 测试Lua虚拟机的所有行为契约，确保100% Lua 5.1.5兼容性
 *              包括字节码执行、堆栈管理、函数调用、异常处理等核心功能
 * @date 2025-09-20
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/generators/catch_generators.hpp>

// 注意：这些头文件还不存在，这是TDD方法 - 先写测试定义接口
#include "vm/virtual_machine.h"
#include "vm/call_frame.h"
#include "vm/stack.h"
#include "compiler/bytecode.h"
#include "types/tvalue.h"
#include "core/lua_common.h"
#include "core/lua_errors.h"

using namespace lua_cpp;
using Catch::Approx;

/* ========================================================================== */
/* VM基础架构契约 */
/* ========================================================================== */

TEST_CASE("VM - 虚拟机初始化契约", "[vm][contract][basic]") {
    SECTION("VM应该正确初始化") {
        VirtualMachine vm;
        
        REQUIRE(vm.GetStackSize() >= VM_MIN_STACK_SIZE);
        REQUIRE(vm.GetStackTop() == 0);
        REQUIRE(vm.GetCallFrameCount() == 0);
        REQUIRE(vm.GetInstructionPointer() == 0);
        REQUIRE(vm.GetExecutionState() == ExecutionState::Ready);
    }

    SECTION("VM状态管理") {
        VirtualMachine vm;
        
        REQUIRE(vm.GetExecutionState() == ExecutionState::Ready);
        
        // 模拟运行状态
        vm.SetExecutionState(ExecutionState::Running);
        REQUIRE(vm.GetExecutionState() == ExecutionState::Running);
        
        // 模拟暂停状态
        vm.SetExecutionState(ExecutionState::Suspended);
        REQUIRE(vm.GetExecutionState() == ExecutionState::Suspended);
        
        // 模拟错误状态
        vm.SetExecutionState(ExecutionState::Error);
        REQUIRE(vm.GetExecutionState() == ExecutionState::Error);
    }

    SECTION("VM配置参数") {
        VMConfig config;
        config.initial_stack_size = 1024;
        config.max_stack_size = 65536;
        config.enable_debug_info = true;
        config.enable_profiling = false;
        
        VirtualMachine vm(config);
        
        REQUIRE(vm.GetStackSize() >= config.initial_stack_size);
        REQUIRE(vm.GetMaxStackSize() == config.max_stack_size);
        REQUIRE(vm.IsDebugEnabled() == config.enable_debug_info);
        REQUIRE(vm.IsProfilingEnabled() == config.enable_profiling);
    }
}

/* ========================================================================== */
/* 堆栈管理契约 */
/* ========================================================================== */

TEST_CASE("VM - 堆栈操作契约", "[vm][contract][stack]") {
    SECTION("基本堆栈操作") {
        VirtualMachine vm;
        
        // 推入值
        vm.Push(TValue::CreateNumber(42.0));
        vm.Push(TValue::CreateString("hello"));
        vm.Push(TValue::CreateBoolean(true));
        
        REQUIRE(vm.GetStackTop() == 3);
        
        // 查看栈顶值
        TValue top = vm.Top();
        REQUIRE(top.IsBoolean());
        REQUIRE(top.GetBoolean() == true);
        
        // 弹出值
        TValue popped = vm.Pop();
        REQUIRE(popped.IsBoolean());
        REQUIRE(vm.GetStackTop() == 2);
        
        // 访问栈中特定位置
        TValue at_index = vm.GetStack(0);
        REQUIRE(at_index.IsNumber());
        REQUIRE(at_index.GetNumber() == Approx(42.0));
    }

    SECTION("堆栈边界检查") {
        VMConfig config;
        config.initial_stack_size = 4;
        config.max_stack_size = 8;
        VirtualMachine vm(config);
        
        // 填满初始栈
        for (int i = 0; i < 4; ++i) {
            vm.Push(TValue::CreateNumber(i));
        }
        
        // 继续推入应该触发栈扩展
        vm.Push(TValue::CreateNumber(4));
        REQUIRE(vm.GetStackSize() > 4);
        REQUIRE(vm.GetStackTop() == 5);
        
        // 超过最大栈大小应该抛出异常
        REQUIRE_THROWS_AS([&]() {
            for (int i = 5; i < 20; ++i) {
                vm.Push(TValue::CreateNumber(i));
            }
        }(), StackOverflowError);
    }

    SECTION("堆栈操作异常处理") {
        VirtualMachine vm;
        
        // 空栈弹出应该抛出异常
        REQUIRE_THROWS_AS(vm.Pop(), StackUnderflowError);
        
        // 访问越界索引应该抛出异常
        REQUIRE_THROWS_AS(vm.GetStack(100), StackIndexError);
        REQUIRE_THROWS_AS(vm.GetStack(-1), StackIndexError);
        
        // 设置越界位置应该抛出异常
        REQUIRE_THROWS_AS(vm.SetStack(100, TValue::CreateNil()), StackIndexError);
    }

    SECTION("堆栈快照和恢复") {
        VirtualMachine vm;
        
        // 设置初始状态
        vm.Push(TValue::CreateNumber(1));
        vm.Push(TValue::CreateNumber(2));
        vm.Push(TValue::CreateNumber(3));
        
        Size saved_top = vm.GetStackTop();
        
        // 修改堆栈
        vm.Push(TValue::CreateString("temp"));
        vm.Push(TValue::CreateBoolean(false));
        
        // 恢复到保存的状态
        vm.SetStackTop(saved_top);
        
        REQUIRE(vm.GetStackTop() == 3);
        REQUIRE(vm.Top().IsNumber());
        REQUIRE(vm.Top().GetNumber() == Approx(3.0));
    }
}

/* ========================================================================== */
/* 调用帧管理契约 */
/* ========================================================================== */

TEST_CASE("VM - 调用帧管理契约", "[vm][contract][callframe]") {
    SECTION("调用帧创建和销毁") {
        VirtualMachine vm;
        
        // 创建一个简单的函数原型
        auto proto = std::make_unique<Proto>();
        proto->SetParameterCount(2);
        proto->AddInstruction(CreateABCInstruction(OpCode::RETURN, 0, 1, 0));
        
        // 创建调用帧
        vm.PushCallFrame(proto.get(), 0, 2); // base=0, param_count=2
        
        REQUIRE(vm.GetCallFrameCount() == 1);
        
        auto& frame = vm.GetCurrentCallFrame();
        REQUIRE(frame.GetProto() == proto.get());
        REQUIRE(frame.GetBase() == 0);
        REQUIRE(frame.GetInstructionPointer() == 0);
        
        // 销毁调用帧
        vm.PopCallFrame();
        REQUIRE(vm.GetCallFrameCount() == 0);
    }

    SECTION("调用帧栈操作") {
        VirtualMachine vm;
        
        auto proto1 = std::make_unique<Proto>();
        auto proto2 = std::make_unique<Proto>();
        auto proto3 = std::make_unique<Proto>();
        
        // 创建嵌套调用帧
        vm.PushCallFrame(proto1.get(), 0, 0);
        vm.PushCallFrame(proto2.get(), 5, 1);
        vm.PushCallFrame(proto3.get(), 10, 2);
        
        REQUIRE(vm.GetCallFrameCount() == 3);
        
        // 检查当前调用帧
        REQUIRE(vm.GetCurrentCallFrame().GetProto() == proto3.get());
        
        // 逐个弹出调用帧
        vm.PopCallFrame();
        REQUIRE(vm.GetCurrentCallFrame().GetProto() == proto2.get());
        
        vm.PopCallFrame();
        REQUIRE(vm.GetCurrentCallFrame().GetProto() == proto1.get());
        
        vm.PopCallFrame();
        REQUIRE(vm.GetCallFrameCount() == 0);
    }

    SECTION("调用帧溢出检查") {
        VirtualMachine vm;
        auto proto = std::make_unique<Proto>();
        
        // 创建过多的调用帧应该抛出异常
        REQUIRE_THROWS_AS([&]() {
            for (int i = 0; i < 1000; ++i) {
                vm.PushCallFrame(proto.get(), i, 0);
            }
        }(), CallStackOverflowError);
    }
}

/* ========================================================================== */
/* 指令执行契约 */
/* ========================================================================== */

TEST_CASE("VM - 数据移动指令执行契约", "[vm][contract][instruction][move]") {
    SECTION("OP_MOVE指令执行") {
        VirtualMachine vm;
        
        // 设置源寄存器
        vm.SetStack(1, TValue::CreateNumber(42.0));
        
        // 执行MOVE指令: R(0) = R(1)
        Instruction move_inst = CreateABCInstruction(OpCode::MOVE, 0, 1, 0);
        vm.ExecuteInstruction(move_inst);
        
        // 验证结果
        TValue result = vm.GetStack(0);
        REQUIRE(result.IsNumber());
        REQUIRE(result.GetNumber() == Approx(42.0));
    }

    SECTION("OP_LOADK指令执行") {
        VirtualMachine vm;
        auto proto = std::make_unique<Proto>();
        
        // 添加常量
        Size const_idx = proto->AddConstant(TValue::CreateString("hello"));
        
        // 设置当前函数
        vm.PushCallFrame(proto.get(), 0, 0);
        
        // 执行LOADK指令: R(0) = K(const_idx)
        Instruction loadk_inst = CreateABxInstruction(OpCode::LOADK, 0, const_idx);
        vm.ExecuteInstruction(loadk_inst);
        
        // 验证结果
        TValue result = vm.GetStack(0);
        REQUIRE(result.IsString());
        REQUIRE(result.GetString() == "hello");
    }

    SECTION("OP_LOADBOOL指令执行") {
        VirtualMachine vm;
        
        // 执行LOADBOOL指令: R(0) = true, skip next
        Instruction loadbool_inst = CreateABCInstruction(OpCode::LOADBOOL, 0, 1, 1);
        Size initial_pc = vm.GetInstructionPointer();
        
        vm.ExecuteInstruction(loadbool_inst);
        
        // 验证结果
        TValue result = vm.GetStack(0);
        REQUIRE(result.IsBoolean());
        REQUIRE(result.GetBoolean() == true);
        
        // 验证跳转
        REQUIRE(vm.GetInstructionPointer() == initial_pc + 2); // 跳过下一条指令
    }

    SECTION("OP_LOADNIL指令执行") {
        VirtualMachine vm;
        
        // 先设置一些非nil值
        vm.SetStack(0, TValue::CreateNumber(1));
        vm.SetStack(1, TValue::CreateNumber(2));
        vm.SetStack(2, TValue::CreateNumber(3));
        
        // 执行LOADNIL指令: R(0) to R(2) = nil
        Instruction loadnil_inst = CreateABCInstruction(OpCode::LOADNIL, 0, 2, 0);
        vm.ExecuteInstruction(loadnil_inst);
        
        // 验证结果
        REQUIRE(vm.GetStack(0).IsNil());
        REQUIRE(vm.GetStack(1).IsNil());
        REQUIRE(vm.GetStack(2).IsNil());
    }
}

TEST_CASE("VM - 算术指令执行契约", "[vm][contract][instruction][arithmetic]") {
    SECTION("OP_ADD指令执行") {
        VirtualMachine vm;
        
        // 设置操作数
        vm.SetStack(1, TValue::CreateNumber(10.0));
        vm.SetStack(2, TValue::CreateNumber(5.0));
        
        // 执行ADD指令: R(0) = R(1) + R(2)
        Instruction add_inst = CreateABCInstruction(OpCode::ADD, 0, 1, 2);
        vm.ExecuteInstruction(add_inst);
        
        // 验证结果
        TValue result = vm.GetStack(0);
        REQUIRE(result.IsNumber());
        REQUIRE(result.GetNumber() == Approx(15.0));
    }

    SECTION("算术指令RK值处理") {
        VirtualMachine vm;
        auto proto = std::make_unique<Proto>();
        
        // 添加常量
        Size const_idx = proto->AddConstant(TValue::CreateNumber(3.0));
        
        vm.PushCallFrame(proto.get(), 0, 0);
        vm.SetStack(1, TValue::CreateNumber(7.0));
        
        // 执行MUL指令: R(0) = R(1) * K(const_idx)
        // 使用RK编码：const_idx | BITRK
        int rk_const = const_idx | (1 << 8); // BITRK = 256
        Instruction mul_inst = CreateABCInstruction(OpCode::MUL, 0, 1, rk_const);
        vm.ExecuteInstruction(mul_inst);
        
        // 验证结果
        TValue result = vm.GetStack(0);
        REQUIRE(result.IsNumber());
        REQUIRE(result.GetNumber() == Approx(21.0)); // 7 * 3
    }

    SECTION("算术指令类型错误处理") {
        VirtualMachine vm;
        
        // 设置不兼容的操作数
        vm.SetStack(1, TValue::CreateString("not a number"));
        vm.SetStack(2, TValue::CreateNumber(5.0));
        
        // 执行ADD指令应该尝试类型转换或触发元方法
        Instruction add_inst = CreateABCInstruction(OpCode::ADD, 0, 1, 2);
        
        // 在没有元方法的情况下应该抛出类型错误
        REQUIRE_THROWS_AS(vm.ExecuteInstruction(add_inst), TypeError);
    }

    SECTION("一元算术指令") {
        VirtualMachine vm;
        
        // UNM (unary minus)
        vm.SetStack(1, TValue::CreateNumber(42.0));
        Instruction unm_inst = CreateABCInstruction(OpCode::UNM, 0, 1, 0);
        vm.ExecuteInstruction(unm_inst);
        
        TValue result = vm.GetStack(0);
        REQUIRE(result.IsNumber());
        REQUIRE(result.GetNumber() == Approx(-42.0));
        
        // LEN (length operator)
        vm.SetStack(1, TValue::CreateString("hello"));
        Instruction len_inst = CreateABCInstruction(OpCode::LEN, 0, 1, 0);
        vm.ExecuteInstruction(len_inst);
        
        result = vm.GetStack(0);
        REQUIRE(result.IsNumber());
        REQUIRE(result.GetNumber() == Approx(5.0));
    }
}

TEST_CASE("VM - 比较和跳转指令契约", "[vm][contract][instruction][comparison]") {
    SECTION("OP_EQ指令执行") {
        VirtualMachine vm;
        
        // 设置相等的值
        vm.SetStack(1, TValue::CreateNumber(42.0));
        vm.SetStack(2, TValue::CreateNumber(42.0));
        
        Size initial_pc = vm.GetInstructionPointer();
        
        // 执行EQ指令: if R(1) == R(2) then skip next
        Instruction eq_inst = CreateABCInstruction(OpCode::EQ, 1, 1, 2); // A=1表示相等时跳转
        vm.ExecuteInstruction(eq_inst);
        
        // 相等时应该跳过下一条指令
        REQUIRE(vm.GetInstructionPointer() == initial_pc + 2);
    }

    SECTION("OP_LT和OP_LE指令执行") {
        VirtualMachine vm;
        
        vm.SetStack(1, TValue::CreateNumber(5.0));
        vm.SetStack(2, TValue::CreateNumber(10.0));
        
        Size initial_pc = vm.GetInstructionPointer();
        
        // 执行LT指令: if R(1) < R(2) then skip next
        Instruction lt_inst = CreateABCInstruction(OpCode::LT, 1, 1, 2);
        vm.ExecuteInstruction(lt_inst);
        
        // 5 < 10为真，应该跳过下一条指令
        REQUIRE(vm.GetInstructionPointer() == initial_pc + 2);
    }

    SECTION("OP_JMP指令执行") {
        VirtualMachine vm;
        
        Size initial_pc = vm.GetInstructionPointer();
        
        // 执行JMP指令: pc += sBx
        int jump_offset = 10;
        Instruction jmp_inst = CreateAsBxInstruction(OpCode::JMP, 0, jump_offset);
        vm.ExecuteInstruction(jmp_inst);
        
        // 验证跳转
        REQUIRE(vm.GetInstructionPointer() == initial_pc + 1 + jump_offset);
    }

    SECTION("OP_TEST指令执行") {
        VirtualMachine vm;
        
        // 测试真值
        vm.SetStack(1, TValue::CreateBoolean(true));
        
        Size initial_pc = vm.GetInstructionPointer();
        
        // 执行TEST指令: if (R(1) != C) then skip next
        Instruction test_inst = CreateABCInstruction(OpCode::TEST, 1, 0, 0); // C=0表示测试假值
        vm.ExecuteInstruction(test_inst);
        
        // R(1)是true，C是0(false)，不相等所以跳转
        REQUIRE(vm.GetInstructionPointer() == initial_pc + 2);
    }
}

/* ========================================================================== */
/* 函数调用契约 */
/* ========================================================================== */

TEST_CASE("VM - 函数调用指令契约", "[vm][contract][instruction][call]") {
    SECTION("OP_CALL指令执行") {
        VirtualMachine vm;
        
        // 创建一个简单的函数
        auto proto = std::make_unique<Proto>();
        proto->SetParameterCount(2);
        proto->AddInstruction(CreateABCInstruction(OpCode::LOADK, 0, 0, 0)); // 返回常量
        proto->AddInstruction(CreateABCInstruction(OpCode::RETURN, 0, 2, 0)); // 返回1个值
        proto->AddConstant(TValue::CreateString("result"));
        
        // 创建函数对象
        TValue func = TValue::CreateFunction(proto.get());
        
        // 设置调用栈：func, arg1, arg2
        vm.SetStack(0, func);
        vm.SetStack(1, TValue::CreateNumber(10.0));
        vm.SetStack(2, TValue::CreateNumber(20.0));
        
        // 执行CALL指令: call R(0) with 2 args, 1 result
        Instruction call_inst = CreateABCInstruction(OpCode::CALL, 0, 3, 2); // A=0, B=3(func+2args), C=2(1result+1)
        vm.ExecuteInstruction(call_inst);
        
        // 验证返回值
        TValue result = vm.GetStack(0);
        REQUIRE(result.IsString());
        REQUIRE(result.GetString() == "result");
    }

    SECTION("OP_RETURN指令执行") {
        VirtualMachine vm;
        auto proto = std::make_unique<Proto>();
        
        // 创建调用帧
        vm.PushCallFrame(proto.get(), 5, 0); // base = 5
        
        // 设置返回值
        vm.SetStack(5, TValue::CreateNumber(42.0));
        vm.SetStack(6, TValue::CreateString("done"));
        
        // 执行RETURN指令: return R(5), R(6)
        Instruction ret_inst = CreateABCInstruction(OpCode::RETURN, 5, 3, 0); // A=5, B=3(2values+1)
        vm.ExecuteInstruction(ret_inst);
        
        // 验证调用帧被弹出
        REQUIRE(vm.GetCallFrameCount() == 0);
        
        // 验证返回值在栈上的正确位置
        REQUIRE(vm.GetStack(5).IsNumber());
        REQUIRE(vm.GetStack(6).IsString());
    }

    SECTION("OP_TAILCALL指令执行") {
        VirtualMachine vm;
        
        auto proto1 = std::make_unique<Proto>();
        auto proto2 = std::make_unique<Proto>();
        
        // 创建调用帧
        vm.PushCallFrame(proto1.get(), 0, 0);
        
        // 设置尾调用
        TValue func = TValue::CreateFunction(proto2.get());
        vm.SetStack(0, func);
        vm.SetStack(1, TValue::CreateNumber(1.0));
        
        Size initial_frame_count = vm.GetCallFrameCount();
        
        // 执行TAILCALL指令
        Instruction tailcall_inst = CreateABCInstruction(OpCode::TAILCALL, 0, 2, 0); // A=0, B=2(func+1arg)
        vm.ExecuteInstruction(tailcall_inst);
        
        // 尾调用应该复用当前调用帧，不增加调用栈深度
        REQUIRE(vm.GetCallFrameCount() == initial_frame_count);
    }
}

/* ========================================================================== */
/* 表操作契约 */
/* ========================================================================== */

TEST_CASE("VM - 表操作指令契约", "[vm][contract][instruction][table]") {
    SECTION("OP_NEWTABLE指令执行") {
        VirtualMachine vm;
        
        // 执行NEWTABLE指令: R(0) = {} (array_size=2, hash_size=1)
        Instruction newtable_inst = CreateABCInstruction(OpCode::NEWTABLE, 0, 2, 1);
        vm.ExecuteInstruction(newtable_inst);
        
        // 验证创建了新表
        TValue result = vm.GetStack(0);
        REQUIRE(result.IsTable());
        
        auto table = result.GetTable();
        REQUIRE(table != nullptr);
        REQUIRE(table->GetArraySize() >= 2);
    }

    SECTION("OP_GETTABLE指令执行") {
        VirtualMachine vm;
        
        // 创建表并设置值
        auto table = std::make_shared<LuaTable>();
        table->Set(TValue::CreateString("key"), TValue::CreateNumber(42.0));
        
        vm.SetStack(1, TValue::CreateTable(table));
        vm.SetStack(2, TValue::CreateString("key"));
        
        // 执行GETTABLE指令: R(0) = R(1)[R(2)]
        Instruction gettable_inst = CreateABCInstruction(OpCode::GETTABLE, 0, 1, 2);
        vm.ExecuteInstruction(gettable_inst);
        
        // 验证结果
        TValue result = vm.GetStack(0);
        REQUIRE(result.IsNumber());
        REQUIRE(result.GetNumber() == Approx(42.0));
    }

    SECTION("OP_SETTABLE指令执行") {
        VirtualMachine vm;
        
        // 创建空表
        auto table = std::make_shared<LuaTable>();
        vm.SetStack(0, TValue::CreateTable(table));
        vm.SetStack(1, TValue::CreateString("key"));
        vm.SetStack(2, TValue::CreateNumber(123.0));
        
        // 执行SETTABLE指令: R(0)[R(1)] = R(2)
        Instruction settable_inst = CreateABCInstruction(OpCode::SETTABLE, 0, 1, 2);
        vm.ExecuteInstruction(settable_inst);
        
        // 验证表中的值
        TValue stored_value = table->Get(TValue::CreateString("key"));
        REQUIRE(stored_value.IsNumber());
        REQUIRE(stored_value.GetNumber() == Approx(123.0));
    }

    SECTION("OP_SETLIST指令执行") {
        VirtualMachine vm;
        
        // 创建表和数组元素
        auto table = std::make_shared<LuaTable>();
        vm.SetStack(0, TValue::CreateTable(table));
        vm.SetStack(1, TValue::CreateNumber(10.0));
        vm.SetStack(2, TValue::CreateNumber(20.0));
        vm.SetStack(3, TValue::CreateNumber(30.0));
        
        // 执行SETLIST指令: R(0)[1..3] = R(1), R(2), R(3)
        Instruction setlist_inst = CreateABCInstruction(OpCode::SETLIST, 0, 3, 1); // A=0, B=3, C=1(batch)
        vm.ExecuteInstruction(setlist_inst);
        
        // 验证数组元素
        REQUIRE(table->Get(TValue::CreateNumber(1.0)).GetNumber() == Approx(10.0));
        REQUIRE(table->Get(TValue::CreateNumber(2.0)).GetNumber() == Approx(20.0));
        REQUIRE(table->Get(TValue::CreateNumber(3.0)).GetNumber() == Approx(30.0));
    }
}

/* ========================================================================== */
/* 循环指令契约 */
/* ========================================================================== */

TEST_CASE("VM - 循环指令契约", "[vm][contract][instruction][loop]") {
    SECTION("OP_FORPREP指令执行") {
        VirtualMachine vm;
        
        // 设置for循环变量: start=1, limit=10, step=1
        vm.SetStack(0, TValue::CreateNumber(1.0));  // start
        vm.SetStack(1, TValue::CreateNumber(10.0)); // limit
        vm.SetStack(2, TValue::CreateNumber(1.0));  // step
        
        Size initial_pc = vm.GetInstructionPointer();
        
        // 执行FORPREP指令: prepare for loop, jump to sBx
        int loop_jump = 5;
        Instruction forprep_inst = CreateAsBxInstruction(OpCode::FORPREP, 0, loop_jump);
        vm.ExecuteInstruction(forprep_inst);
        
        // 验证初始值设置和跳转
        REQUIRE(vm.GetStack(0).GetNumber() == Approx(0.0)); // start - step
        REQUIRE(vm.GetInstructionPointer() == initial_pc + 1 + loop_jump);
    }

    SECTION("OP_FORLOOP指令执行") {
        VirtualMachine vm;
        
        // 设置for循环状态
        vm.SetStack(0, TValue::CreateNumber(1.0));  // current (start - step + step = start)
        vm.SetStack(1, TValue::CreateNumber(10.0)); // limit
        vm.SetStack(2, TValue::CreateNumber(1.0));  // step
        
        Size initial_pc = vm.GetInstructionPointer();
        
        // 执行FORLOOP指令: increment and test
        int loop_back = -3;
        Instruction forloop_inst = CreateAsBxInstruction(OpCode::FORLOOP, 0, loop_back);
        vm.ExecuteInstruction(forloop_inst);
        
        // 验证递增和循环变量
        REQUIRE(vm.GetStack(0).GetNumber() == Approx(2.0)); // current + step
        REQUIRE(vm.GetStack(3).GetNumber() == Approx(2.0)); // loop variable
        
        // 由于2 <= 10，应该回跳
        REQUIRE(vm.GetInstructionPointer() == initial_pc + 1 + loop_back);
    }

    SECTION("OP_TFORLOOP指令执行") {
        VirtualMachine vm;
        
        // 模拟泛型for循环的迭代器函数
        auto iter_proto = std::make_unique<Proto>();
        iter_proto->AddInstruction(CreateABCInstruction(OpCode::RETURN, 0, 1, 0)); // 返回nil表示结束
        
        TValue iterator = TValue::CreateFunction(iter_proto.get());
        vm.SetStack(0, iterator);  // iterator function
        vm.SetStack(1, TValue::CreateNil()); // state
        vm.SetStack(2, TValue::CreateNil()); // control variable
        
        Size initial_pc = vm.GetInstructionPointer();
        
        // 执行TFORLOOP指令
        int loop_jump = 5;
        Instruction tforloop_inst = CreateAsBxInstruction(OpCode::TFORLOOP, 0, loop_jump);
        vm.ExecuteInstruction(tforloop_inst);
        
        // 由于迭代器返回nil，应该跳出循环
        REQUIRE(vm.GetInstructionPointer() == initial_pc + 1 + loop_jump);
    }
}

/* ========================================================================== */
/* 错误处理和异常契约 */
/* ========================================================================== */

TEST_CASE("VM - 错误处理契约", "[vm][contract][error]") {
    SECTION("运行时错误捕获") {
        VirtualMachine vm;
        
        // 设置除零错误
        vm.SetStack(1, TValue::CreateNumber(10.0));
        vm.SetStack(2, TValue::CreateNumber(0.0));
        
        Instruction div_inst = CreateABCInstruction(OpCode::DIV, 0, 1, 2);
        
        // 除零应该抛出运行时错误
        REQUIRE_THROWS_AS(vm.ExecuteInstruction(div_inst), RuntimeError);
    }

    SECTION("非法指令错误") {
        VirtualMachine vm;
        
        // 创建无效指令
        Instruction invalid_inst = 0xFFFFFFFF; // 无效的操作码
        
        REQUIRE_THROWS_AS(vm.ExecuteInstruction(invalid_inst), InvalidInstructionError);
    }

    SECTION("错误状态传播") {
        VirtualMachine vm;
        
        // 设置错误状态
        vm.SetExecutionState(ExecutionState::Error);
        
        // 在错误状态下执行指令应该失败
        Instruction move_inst = CreateABCInstruction(OpCode::MOVE, 0, 1, 0);
        REQUIRE_THROWS_AS(vm.ExecuteInstruction(move_inst), VMExecutionError);
    }

    SECTION("异常恢复") {
        VirtualMachine vm;
        
        try {
            // 触发错误
            vm.SetStack(1, TValue::CreateString("not a number"));
            vm.SetStack(2, TValue::CreateNumber(5.0));
            Instruction add_inst = CreateABCInstruction(OpCode::ADD, 0, 1, 2);
            vm.ExecuteInstruction(add_inst);
        } catch (const TypeError&) {
            // 捕获错误并重置VM状态
            vm.SetExecutionState(ExecutionState::Ready);
        }
        
        // VM应该能够继续正常执行
        vm.SetStack(1, TValue::CreateNumber(3.0));
        vm.SetStack(2, TValue::CreateNumber(4.0));
        Instruction add_inst = CreateABCInstruction(OpCode::ADD, 0, 1, 2);
        
        REQUIRE_NOTHROW(vm.ExecuteInstruction(add_inst));
        REQUIRE(vm.GetStack(0).GetNumber() == Approx(7.0));
    }
}

/* ========================================================================== */
/* 完整程序执行契约 */
/* ========================================================================== */

TEST_CASE("VM - 完整程序执行契约", "[vm][contract][execution]") {
    SECTION("简单程序执行") {
        VirtualMachine vm;
        
        // 创建程序: local x = 10; local y = 20; return x + y
        auto proto = std::make_unique<Proto>();
        
        // 添加常量
        Size const_10 = proto->AddConstant(TValue::CreateNumber(10.0));
        Size const_20 = proto->AddConstant(TValue::CreateNumber(20.0));
        
        // 生成指令
        proto->AddInstruction(CreateABxInstruction(OpCode::LOADK, 0, const_10));  // R(0) = 10
        proto->AddInstruction(CreateABxInstruction(OpCode::LOADK, 1, const_20));  // R(1) = 20
        proto->AddInstruction(CreateABCInstruction(OpCode::ADD, 2, 0, 1));        // R(2) = R(0) + R(1)
        proto->AddInstruction(CreateABCInstruction(OpCode::RETURN, 2, 2, 0));     // return R(2)
        
        // 执行程序
        auto result = vm.ExecuteProgram(proto.get());
        
        REQUIRE(result.size() == 1);
        REQUIRE(result[0].IsNumber());
        REQUIRE(result[0].GetNumber() == Approx(30.0));
    }

    SECTION("函数调用程序执行") {
        VirtualMachine vm;
        
        // 创建被调用函数: function add(a, b) return a + b end
        auto add_proto = std::make_unique<Proto>();
        add_proto->SetParameterCount(2);
        add_proto->AddInstruction(CreateABCInstruction(OpCode::ADD, 2, 0, 1));    // R(2) = R(0) + R(1)
        add_proto->AddInstruction(CreateABCInstruction(OpCode::RETURN, 2, 2, 0)); // return R(2)
        
        // 创建主程序: return add(5, 7)
        auto main_proto = std::make_unique<Proto>();
        
        Size sub_idx = main_proto->AddSubProto(std::move(add_proto));
        Size const_5 = main_proto->AddConstant(TValue::CreateNumber(5.0));
        Size const_7 = main_proto->AddConstant(TValue::CreateNumber(7.0));
        
        main_proto->AddInstruction(CreateABxInstruction(OpCode::CLOSURE, 0, sub_idx));  // R(0) = closure
        main_proto->AddInstruction(CreateABxInstruction(OpCode::LOADK, 1, const_5));    // R(1) = 5
        main_proto->AddInstruction(CreateABxInstruction(OpCode::LOADK, 2, const_7));    // R(2) = 7
        main_proto->AddInstruction(CreateABCInstruction(OpCode::CALL, 0, 3, 2));        // R(0) = call R(0)(R(1), R(2))
        main_proto->AddInstruction(CreateABCInstruction(OpCode::RETURN, 0, 2, 0));      // return R(0)
        
        // 执行程序
        auto result = vm.ExecuteProgram(main_proto.get());
        
        REQUIRE(result.size() == 1);
        REQUIRE(result[0].IsNumber());
        REQUIRE(result[0].GetNumber() == Approx(12.0));
    }

    SECTION("控制流程序执行") {
        VirtualMachine vm;
        
        // 创建程序: if true then return 1 else return 2 end
        auto proto = std::make_unique<Proto>();
        
        Size const_1 = proto->AddConstant(TValue::CreateNumber(1.0));
        Size const_2 = proto->AddConstant(TValue::CreateNumber(2.0));
        
        proto->AddInstruction(CreateABCInstruction(OpCode::LOADBOOL, 0, 1, 0));   // R(0) = true
        proto->AddInstruction(CreateABCInstruction(OpCode::TEST, 0, 0, 0));       // test R(0)
        proto->AddInstruction(CreateAsBxInstruction(OpCode::JMP, 0, 2));          // jump to else
        proto->AddInstruction(CreateABxInstruction(OpCode::LOADK, 1, const_1));   // R(1) = 1
        proto->AddInstruction(CreateABCInstruction(OpCode::RETURN, 1, 2, 0));     // return R(1)
        proto->AddInstruction(CreateABxInstruction(OpCode::LOADK, 1, const_2));   // R(1) = 2
        proto->AddInstruction(CreateABCInstruction(OpCode::RETURN, 1, 2, 0));     // return R(1)
        
        // 执行程序
        auto result = vm.ExecuteProgram(proto.get());
        
        REQUIRE(result.size() == 1);
        REQUIRE(result[0].IsNumber());
        REQUIRE(result[0].GetNumber() == Approx(1.0)); // 条件为真，返回1
    }
}

/* ========================================================================== */
/* 性能和调试契约 */
/* ========================================================================== */

TEST_CASE("VM - 性能和调试契约", "[vm][contract][performance]") {
    SECTION("指令计数和统计") {
        VMConfig config;
        config.enable_profiling = true;
        VirtualMachine vm(config);
        
        // 执行一些指令
        vm.SetStack(1, TValue::CreateNumber(10.0));
        Instruction move_inst = CreateABCInstruction(OpCode::MOVE, 0, 1, 0);
        vm.ExecuteInstruction(move_inst);
        
        Instruction loadk_inst = CreateABxInstruction(OpCode::LOADK, 2, 0);
        vm.ExecuteInstruction(loadk_inst);
        
        // 检查统计信息
        auto stats = vm.GetExecutionStatistics();
        REQUIRE(stats.total_instructions >= 2);
        REQUIRE(stats.instruction_counts[static_cast<int>(OpCode::MOVE)] >= 1);
    }

    SECTION("调试信息跟踪") {
        VMConfig config;
        config.enable_debug_info = true;
        VirtualMachine vm(config);
        
        // 设置调试钩子
        bool hook_called = false;
        vm.SetDebugHook([&hook_called](const DebugInfo& info) {
            hook_called = true;
            REQUIRE(info.instruction_pointer >= 0);
            REQUIRE(info.current_function != nullptr);
        });
        
        // 执行指令
        vm.SetStack(1, TValue::CreateNumber(42.0));
        Instruction move_inst = CreateABCInstruction(OpCode::MOVE, 0, 1, 0);
        vm.ExecuteInstruction(move_inst);
        
        REQUIRE(hook_called);
    }

    SECTION("内存使用监控") {
        VirtualMachine vm;
        
        Size initial_memory = vm.GetMemoryUsage();
        
        // 分配一些对象
        for (int i = 0; i < 100; ++i) {
            auto table = std::make_shared<LuaTable>();
            vm.Push(TValue::CreateTable(table));
        }
        
        Size final_memory = vm.GetMemoryUsage();
        REQUIRE(final_memory > initial_memory);
    }
}