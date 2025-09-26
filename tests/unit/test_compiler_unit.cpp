/**
 * @file test_compiler_unit.cpp
 * @brief 编译器单元测试
 * @description 基于契约测试验证编译器功能的正确性
 * @date 2025-09-25
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

// 编译器相关头文件
#include "compiler/compiler.h"
#include "compiler/bytecode_generator.h"
#include "compiler/constant_pool.h"
#include "compiler/register_allocator.h"
#include "core/lua_value.h"
#include "core/lua_errors.h"

using namespace lua_cpp;
using Catch::Approx;

/* ========================================================================== */
/* 字节码生成器测试 */
/* ========================================================================== */

TEST_CASE("BytecodeGenerator - 基本指令生成", "[compiler][unit][bytecode_generator]") {
    BytecodeGenerator generator;
    
    SECTION("ABC格式指令") {
        Size pc = generator.EmitABC(OpCode::MOVE, 0, 1, 0);
        REQUIRE(pc == 0);
        
        const auto& instructions = generator.GetInstructions();
        REQUIRE(instructions.size() == 1);
        
        Instruction inst = instructions[0];
        REQUIRE(GetOpCode(inst) == OpCode::MOVE);
        REQUIRE(GetArgA(inst) == 0);
        REQUIRE(GetArgB(inst) == 1);
        REQUIRE(GetArgC(inst) == 0);
    }
    
    SECTION("ABx格式指令") {
        Size pc = generator.EmitABx(OpCode::LOADK, 0, 42);
        REQUIRE(pc == 0);
        
        const auto& instructions = generator.GetInstructions();
        REQUIRE(instructions.size() == 1);
        
        Instruction inst = instructions[0];
        REQUIRE(GetOpCode(inst) == OpCode::LOADK);
        REQUIRE(GetArgA(inst) == 0);
        REQUIRE(GetArgBx(inst) == 42);
    }
    
    SECTION("AsBx格式指令") {
        Size pc = generator.EmitAsBx(OpCode::JMP, 0, -10);
        REQUIRE(pc == 0);
        
        const auto& instructions = generator.GetInstructions();
        REQUIRE(instructions.size() == 1);
        
        Instruction inst = instructions[0];
        REQUIRE(GetOpCode(inst) == OpCode::JMP);
        REQUIRE(GetArgA(inst) == 0);
        REQUIRE(GetArgsBx(inst) == -10);
    }
}

TEST_CASE("BytecodeGenerator - 跳转管理", "[compiler][unit][bytecode_generator]") {
    BytecodeGenerator generator;
    
    SECTION("跳转指令修复") {
        // 发射一个跳转占位符
        Size jump_pc = generator.EmitJump(OpCode::JMP);
        
        // 发射一些其他指令
        generator.EmitABC(OpCode::LOADNIL, 0, 0, 0);
        generator.EmitABC(OpCode::LOADNIL, 1, 1, 0);
        
        Size target_pc = generator.GetCurrentPC();
        
        // 修复跳转
        generator.PatchJump(jump_pc, target_pc);
        
        // 验证跳转指令
        Instruction inst = generator.GetInstruction(jump_pc);
        REQUIRE(GetOpCode(inst) == OpCode::JMP);
        REQUIRE(GetArgsBx(inst) == static_cast<int>(target_pc - jump_pc - 1));
    }
    
    SECTION("跳转到当前位置") {
        Size jump_pc = generator.EmitJump(OpCode::JMP);
        generator.EmitABC(OpCode::LOADNIL, 0, 0, 0);
        
        Size current_pc = generator.GetCurrentPC();
        generator.PatchJumpToHere(jump_pc);
        
        Instruction inst = generator.GetInstruction(jump_pc);
        REQUIRE(GetArgsBx(inst) == static_cast<int>(current_pc - jump_pc - 1));
    }
}

TEST_CASE("BytecodeGenerator - 行号信息", "[compiler][unit][bytecode_generator]") {
    BytecodeGenerator generator;
    
    SECTION("行号记录") {
        generator.SetCurrentLine(10);
        generator.EmitABC(OpCode::LOADK, 0, 0, 0);
        
        generator.SetCurrentLine(20);
        generator.EmitABC(OpCode::RETURN, 0, 1, 0);
        
        const auto& line_info = generator.GetLineInfo();
        REQUIRE(line_info.size() == 2);
        REQUIRE(line_info[0] == 10);
        REQUIRE(line_info[1] == 20);
    }
}

/* ========================================================================== */
/* 常量池测试 */
/* ========================================================================== */

TEST_CASE("ConstantPool - 基本功能", "[compiler][unit][constant_pool]") {
    ConstantPool pool;
    
    SECTION("添加和查找常量") {
        // 添加不同类型的常量
        int nil_idx = pool.AddNil();
        int bool_idx = pool.AddBoolean(true);
        int num_idx = pool.AddNumber(42.5);
        int str_idx = pool.AddString("hello");
        
        REQUIRE(nil_idx == 0);
        REQUIRE(bool_idx == 1);
        REQUIRE(num_idx == 2);
        REQUIRE(str_idx == 3);
        REQUIRE(pool.GetSize() == 4);
        
        // 验证常量值
        REQUIRE(pool.GetConstant(nil_idx).IsNil());
        REQUIRE(pool.GetConstant(bool_idx).AsBool() == true);
        REQUIRE(pool.GetConstant(num_idx).AsNumber() == Approx(42.5));
        REQUIRE(pool.GetConstant(str_idx).AsString() == "hello");
    }
    
    SECTION("常量去重") {
        // 添加相同的常量应该返回相同的索引
        int idx1 = pool.AddNumber(3.14);
        int idx2 = pool.AddNumber(3.14);
        
        REQUIRE(idx1 == idx2);
        REQUIRE(pool.GetSize() == 1);
    }
    
    SECTION("查找不存在的常量") {
        int idx = pool.FindConstant(LuaValue::CreateNumber(999));
        REQUIRE(idx == -1);
    }
}

TEST_CASE("ConstantPool - 类型查找", "[compiler][unit][constant_pool]") {
    ConstantPool pool;
    
    // 添加各种类型的常量
    pool.AddNumber(1.0);
    pool.AddString("a");
    pool.AddNumber(2.0);
    pool.AddString("b");
    pool.AddBoolean(true);
    
    SECTION("按类型查找") {
        auto numbers = pool.FindConstantsByType(LuaType::Number);
        auto strings = pool.FindConstantsByType(LuaType::String);
        auto bools = pool.FindConstantsByType(LuaType::Bool);
        
        REQUIRE(numbers.size() == 2);
        REQUIRE(strings.size() == 2);
        REQUIRE(bools.size() == 1);
        
        REQUIRE(numbers[0] == 0);
        REQUIRE(numbers[1] == 2);
        REQUIRE(strings[0] == 1);
        REQUIRE(strings[1] == 3);
        REQUIRE(bools[0] == 4);
    }
}

TEST_CASE("ConstantPoolBuilder - 构建器功能", "[compiler][unit][constant_pool]") {
    ConstantPoolBuilder builder;
    
    SECTION("查找或添加") {
        // 第一次添加
        int idx1 = builder.FindOrAddNumber(42.0);
        REQUIRE(idx1 == 0);
        
        // 第二次应该找到现有的
        int idx2 = builder.FindOrAddNumber(42.0);
        REQUIRE(idx2 == 0);
        REQUIRE(builder.GetSize() == 1);
    }
    
    SECTION("构建最终常量池") {
        builder.AddString("test");
        builder.AddNumber(123);
        builder.AddBoolean(false);
        
        ConstantPool pool = std::move(builder).Build();
        
        REQUIRE(pool.GetSize() == 3);
        REQUIRE(pool.GetConstant(0).AsString() == "test");
        REQUIRE(pool.GetConstant(1).AsNumber() == Approx(123));
        REQUIRE(pool.GetConstant(2).AsBool() == false);
    }
}

/* ========================================================================== */
/* 寄存器分配器测试 */
/* ========================================================================== */

TEST_CASE("RegisterAllocator - 基本分配", "[compiler][unit][register_allocator]") {
    RegisterAllocator allocator(10); // 限制为10个寄存器
    
    SECTION("顺序分配") {
        RegisterIndex reg1 = allocator.Allocate();
        RegisterIndex reg2 = allocator.Allocate();
        RegisterIndex reg3 = allocator.Allocate();
        
        REQUIRE(reg1 == 0);
        REQUIRE(reg2 == 1);
        REQUIRE(reg3 == 2);
        REQUIRE(allocator.GetTop() == 3);
        REQUIRE(allocator.GetUsedCount() == 3);
        REQUIRE(allocator.GetFreeCount() == 7);
    }
    
    SECTION("释放和重用") {
        RegisterIndex reg1 = allocator.Allocate();
        RegisterIndex reg2 = allocator.Allocate();
        
        allocator.Free(reg1);
        
        RegisterIndex reg3 = allocator.Allocate();
        REQUIRE(reg3 == reg1); // 应该重用释放的寄存器
    }
    
    SECTION("临时寄存器") {
        RegisterIndex temp1 = allocator.AllocateTemporary();
        RegisterIndex temp2 = allocator.AllocateTemporary();
        
        REQUIRE(allocator.IsTemporary(temp1));
        REQUIRE(allocator.IsTemporary(temp2));
        
        Size saved_top = allocator.SaveTempTop();
        RegisterIndex temp3 = allocator.AllocateTemporary();
        
        allocator.RestoreTempTop();
        
        REQUIRE(allocator.IsFree(temp3));
        REQUIRE(allocator.IsAllocated(temp1));
        REQUIRE(allocator.IsAllocated(temp2));
    }
}

TEST_CASE("RegisterAllocator - 连续分配", "[compiler][unit][register_allocator]") {
    RegisterAllocator allocator(10);
    
    SECTION("分配连续寄存器") {
        RegisterIndex start = allocator.AllocateRange(3);
        
        REQUIRE(start == 0);
        REQUIRE(allocator.IsAllocated(0));
        REQUIRE(allocator.IsAllocated(1));
        REQUIRE(allocator.IsAllocated(2));
        REQUIRE(allocator.GetTop() == 3);
    }
    
    SECTION("释放连续寄存器") {
        RegisterIndex start = allocator.AllocateRange(3);
        allocator.FreeRange(start, 3);
        
        REQUIRE(allocator.IsFree(0));
        REQUIRE(allocator.IsFree(1));
        REQUIRE(allocator.IsFree(2));
    }
}

TEST_CASE("RegisterAllocator - 寄存器信息", "[compiler][unit][register_allocator]") {
    RegisterAllocator allocator(10);
    
    SECTION("命名寄存器") {
        RegisterIndex reg = allocator.AllocateNamed("test_var");
        
        REQUIRE(allocator.GetRegisterName(reg) == "test_var");
        REQUIRE(allocator.GetRegisterType(reg) == RegisterType::Local);
    }
    
    SECTION("设置寄存器名称") {
        RegisterIndex reg = allocator.Allocate();
        allocator.SetRegisterName(reg, "custom_name");
        
        REQUIRE(allocator.GetRegisterName(reg) == "custom_name");
    }
}

/* ========================================================================== */
/* 作用域管理器测试 */
/* ========================================================================== */

TEST_CASE("ScopeManager - 作用域管理", "[compiler][unit][scope_manager]") {
    ScopeManager scope_manager;
    RegisterAllocator allocator;
    
    SECTION("嵌套作用域") {
        REQUIRE(scope_manager.GetCurrentLevel() == 0);
        
        scope_manager.EnterScope();
        REQUIRE(scope_manager.GetCurrentLevel() == 1);
        
        scope_manager.EnterScope();
        REQUIRE(scope_manager.GetCurrentLevel() == 2);
        
        int removed = scope_manager.ExitScope();
        REQUIRE(scope_manager.GetCurrentLevel() == 1);
        REQUIRE(removed == 0); // 没有变量被移除
        
        scope_manager.ExitScope();
        REQUIRE(scope_manager.GetCurrentLevel() == 0);
    }
    
    SECTION("局部变量声明") {
        scope_manager.EnterScope();
        
        RegisterIndex reg1 = scope_manager.DeclareLocal("var1", allocator);
        RegisterIndex reg2 = scope_manager.DeclareLocal("var2", allocator);
        
        REQUIRE(scope_manager.IsLocalDeclared("var1"));
        REQUIRE(scope_manager.IsLocalDeclared("var2"));
        REQUIRE_FALSE(scope_manager.IsLocalDeclared("var3"));
        
        REQUIRE(scope_manager.GetLocalRegister("var1") == reg1);
        REQUIRE(scope_manager.GetLocalRegister("var2") == reg2);
        REQUIRE(scope_manager.GetLocalCount() == 2);
        
        int removed = scope_manager.ExitScope();
        REQUIRE(removed == 2);
        REQUIRE(scope_manager.GetLocalCount() == 0);
    }
    
    SECTION("变量查找") {
        scope_manager.EnterScope();
        RegisterIndex reg = scope_manager.DeclareLocal("test", allocator);
        
        const LocalVariable* local = scope_manager.FindLocal("test");
        REQUIRE(local != nullptr);
        REQUIRE(local->name == "test");
        REQUIRE(local->register_idx == reg);
        REQUIRE(local->scope_level == 1);
        
        const LocalVariable* not_found = scope_manager.FindLocal("not_exists");
        REQUIRE(not_found == nullptr);
        
        scope_manager.ExitScope();
    }
}

TEST_CASE("ScopeManager - 闭包相关", "[compiler][unit][scope_manager]") {
    ScopeManager scope_manager;
    RegisterAllocator allocator;
    
    SECTION("变量捕获") {
        scope_manager.EnterScope();
        scope_manager.DeclareLocal("captured_var", allocator);
        
        REQUIRE_FALSE(scope_manager.IsCaptured("captured_var"));
        
        scope_manager.MarkCaptured("captured_var");
        REQUIRE(scope_manager.IsCaptured("captured_var"));
        
        scope_manager.ExitScope();
    }
}

/* ========================================================================== */
/* 指令发射器测试 */
/* ========================================================================== */

TEST_CASE("InstructionEmitter - 数据移动指令", "[compiler][unit][instruction_emitter]") {
    BytecodeGenerator generator;
    InstructionEmitter emitter(generator);
    
    SECTION("MOVE指令") {
        Size pc = emitter.EmitMove(1, 0);
        
        Instruction inst = generator.GetInstruction(pc);
        REQUIRE(GetOpCode(inst) == OpCode::MOVE);
        REQUIRE(GetArgA(inst) == 1);
        REQUIRE(GetArgB(inst) == 0);
        REQUIRE(GetArgC(inst) == 0);
    }
    
    SECTION("LOADK指令") {
        Size pc = emitter.EmitLoadK(0, 5);
        
        Instruction inst = generator.GetInstruction(pc);
        REQUIRE(GetOpCode(inst) == OpCode::LOADK);
        REQUIRE(GetArgA(inst) == 0);
        REQUIRE(GetArgBx(inst) == 5);
    }
    
    SECTION("LOADBOOL指令") {
        Size pc1 = emitter.EmitLoadBool(0, true, false);
        Size pc2 = emitter.EmitLoadBool(1, false, true);
        
        Instruction inst1 = generator.GetInstruction(pc1);
        REQUIRE(GetOpCode(inst1) == OpCode::LOADBOOL);
        REQUIRE(GetArgA(inst1) == 0);
        REQUIRE(GetArgB(inst1) == 1);
        REQUIRE(GetArgC(inst1) == 0);
        
        Instruction inst2 = generator.GetInstruction(pc2);
        REQUIRE(GetArgB(inst2) == 0);
        REQUIRE(GetArgC(inst2) == 1);
    }
    
    SECTION("LOADNIL指令") {
        Size pc = emitter.EmitLoadNil(0, 2);
        
        Instruction inst = generator.GetInstruction(pc);
        REQUIRE(GetOpCode(inst) == OpCode::LOADNIL);
        REQUIRE(GetArgA(inst) == 0);
        REQUIRE(GetArgB(inst) == 2);
    }
}

TEST_CASE("InstructionEmitter - 算术指令", "[compiler][unit][instruction_emitter]") {
    BytecodeGenerator generator;
    InstructionEmitter emitter(generator);
    
    SECTION("ADD指令") {
        Size pc = emitter.EmitAdd(2, 0, 1);
        
        Instruction inst = generator.GetInstruction(pc);
        REQUIRE(GetOpCode(inst) == OpCode::ADD);
        REQUIRE(GetArgA(inst) == 2);
        REQUIRE(GetArgB(inst) == 0);
        REQUIRE(GetArgC(inst) == 1);
    }
    
    SECTION("一元运算指令") {
        Size pc1 = emitter.EmitUnm(1, 0);
        Size pc2 = emitter.EmitNot(2, 1);
        Size pc3 = emitter.EmitLen(3, 2);
        
        Instruction inst1 = generator.GetInstruction(pc1);
        REQUIRE(GetOpCode(inst1) == OpCode::UNM);
        
        Instruction inst2 = generator.GetInstruction(pc2);
        REQUIRE(GetOpCode(inst2) == OpCode::NOT);
        
        Instruction inst3 = generator.GetInstruction(pc3);
        REQUIRE(GetOpCode(inst3) == OpCode::LEN);
    }
}

TEST_CASE("InstructionEmitter - 函数调用指令", "[compiler][unit][instruction_emitter]") {
    BytecodeGenerator generator;
    InstructionEmitter emitter(generator);
    
    SECTION("CALL指令") {
        Size pc = emitter.EmitCall(0, 2, 1); // 调用R(0)，1个参数，0个返回值
        
        Instruction inst = generator.GetInstruction(pc);
        REQUIRE(GetOpCode(inst) == OpCode::CALL);
        REQUIRE(GetArgA(inst) == 0);
        REQUIRE(GetArgB(inst) == 2);
        REQUIRE(GetArgC(inst) == 1);
    }
    
    SECTION("RETURN指令") {
        Size pc = emitter.EmitReturn(0, 2); // 返回R(0)开始的1个值
        
        Instruction inst = generator.GetInstruction(pc);
        REQUIRE(GetOpCode(inst) == OpCode::RETURN);
        REQUIRE(GetArgA(inst) == 0);
        REQUIRE(GetArgB(inst) == 2);
    }
}

/* ========================================================================== */
/* 常量折叠测试 */
/* ========================================================================== */

TEST_CASE("常量折叠 - 二元运算", "[compiler][unit][constant_folding]") {
    SECTION("算术运算") {
        LuaValue left = LuaValue::CreateNumber(10.0);
        LuaValue right = LuaValue::CreateNumber(3.0);
        
        LuaValue add_result = FoldConstants(left, right, OpCode::ADD);
        REQUIRE(add_result.AsNumber() == Approx(13.0));
        
        LuaValue sub_result = FoldConstants(left, right, OpCode::SUB);
        REQUIRE(sub_result.AsNumber() == Approx(7.0));
        
        LuaValue mul_result = FoldConstants(left, right, OpCode::MUL);
        REQUIRE(mul_result.AsNumber() == Approx(30.0));
        
        LuaValue div_result = FoldConstants(left, right, OpCode::DIV);
        REQUIRE(div_result.AsNumber() == Approx(10.0/3.0));
        
        LuaValue pow_result = FoldConstants(left, right, OpCode::POW);
        REQUIRE(pow_result.AsNumber() == Approx(1000.0));
    }
    
    SECTION("除零保护") {
        LuaValue left = LuaValue::CreateNumber(10.0);
        LuaValue zero = LuaValue::CreateNumber(0.0);
        
        LuaValue result = FoldConstants(left, zero, OpCode::DIV);
        REQUIRE(result.IsNil()); // 应该返回nil表示无法折叠
    }
}

TEST_CASE("常量折叠 - 一元运算", "[compiler][unit][constant_folding]") {
    SECTION("数值运算") {
        LuaValue num = LuaValue::CreateNumber(42.0);
        
        LuaValue unm_result = FoldUnaryConstant(num, OpCode::UNM);
        REQUIRE(unm_result.AsNumber() == Approx(-42.0));
    }
    
    SECTION("逻辑运算") {
        LuaValue true_val = LuaValue::CreateBool(true);
        LuaValue false_val = LuaValue::CreateBool(false);
        LuaValue nil_val = LuaValue::CreateNil();
        LuaValue num_val = LuaValue::CreateNumber(5.0);
        
        REQUIRE(FoldUnaryConstant(true_val, OpCode::NOT).AsBool() == false);
        REQUIRE(FoldUnaryConstant(false_val, OpCode::NOT).AsBool() == true);
        REQUIRE(FoldUnaryConstant(nil_val, OpCode::NOT).AsBool() == true);
        REQUIRE(FoldUnaryConstant(num_val, OpCode::NOT).AsBool() == false); // 数字为真
    }
    
    SECTION("字符串长度") {
        LuaValue str = LuaValue::CreateString("hello");
        
        LuaValue len_result = FoldUnaryConstant(str, OpCode::LEN);
        REQUIRE(len_result.AsNumber() == Approx(5.0));
    }
}

/* ========================================================================== */
/* RK值编码测试 */
/* ========================================================================== */

TEST_CASE("RK值编码 - 基本功能", "[compiler][unit][rk_encoding]") {
    SECTION("寄存器编码") {
        int rk = EncodeRK(static_cast<RegisterIndex>(5));
        
        REQUIRE_FALSE(IsConstant(rk));
        REQUIRE(RKToRegisterIndex(rk) == 5);
    }
    
    SECTION("常量编码") {
        int rk = EncodeRK(10); // 常量索引
        
        // 注意：这里需要正确的常量编码
        rk = ConstantIndexToRK(10);
        
        REQUIRE(IsConstant(rk));
        REQUIRE(RKToConstantIndex(rk) == 10);
    }
    
    SECTION("有效性检查") {
        REQUIRE(IsValidRegister(0));
        REQUIRE(IsValidRegister(255));
        REQUIRE_FALSE(IsValidRegister(256));
        
        REQUIRE(IsValidConstantIndex(0));
        REQUIRE(IsValidConstantIndex(511));
        REQUIRE_FALSE(IsValidConstantIndex(512));
    }
}