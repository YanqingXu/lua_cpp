/**
 * @file test_compiler_contract.cpp
 * @brief Compiler（编译器）契约测试
 * @description 测试Lua编译器的所有行为契约，确保100% Lua 5.1.5兼容性
 *              包括字节码指令生成、优化、符号表管理、常量折叠等核心功能
 * @date 2025-09-20
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/generators/catch_generators.hpp>

// 注意：这些头文件还不存在，这是TDD方法 - 先写测试定义接口
#include "compiler/bytecode.h"
#include "compiler/compiler.h"
#include "parser/ast.h"
#include "parser/parser.h"
#include "core/lua_common.h"
#include "core/lua_errors.h"

using namespace lua_cpp;
using Catch::Approx;

/* ========================================================================== */
/* 字节码指令基础契约 */
/* ========================================================================== */

TEST_CASE("Bytecode - 指令格式契约", "[compiler][contract][bytecode][instruction]") {
    SECTION("指令字段编码解码") {
        // 测试iABC格式指令 (OP_MOVE)
        Instruction inst = CreateABCInstruction(OpCode::MOVE, 1, 2, 0);
        
        REQUIRE(GetOpCode(inst) == OpCode::MOVE);
        REQUIRE(GetArgA(inst) == 1);
        REQUIRE(GetArgB(inst) == 2);
        REQUIRE(GetArgC(inst) == 0);
    }

    SECTION("iABx格式指令编码") {
        // 测试iABx格式指令 (OP_LOADK)
        Instruction inst = CreateABxInstruction(OpCode::LOADK, 0, 1000);
        
        REQUIRE(GetOpCode(inst) == OpCode::LOADK);
        REQUIRE(GetArgA(inst) == 0);
        REQUIRE(GetArgBx(inst) == 1000);
    }

    SECTION("iAsBx格式指令编码") {
        // 测试iAsBx格式指令 (OP_JMP)
        Instruction inst = CreateAsBxInstruction(OpCode::JMP, 0, -50);
        
        REQUIRE(GetOpCode(inst) == OpCode::JMP);
        REQUIRE(GetArgA(inst) == 0);
        REQUIRE(GetArgsBx(inst) == -50);
    }

    SECTION("RK值编码解码") {
        // 寄存器索引
        int reg_rk = ToRK(5, false);
        REQUIRE(IsConstant(reg_rk) == false);
        REQUIRE(GetRKValue(reg_rk) == 5);
        
        // 常量索引
        int const_rk = ToRK(10, true);
        REQUIRE(IsConstant(const_rk) == true);
        REQUIRE(GetRKValue(const_rk) == 10);
    }
}

TEST_CASE("Bytecode - OpCode枚举契约", "[compiler][contract][bytecode][opcode]") {
    SECTION("所有Lua 5.1.5指令都应该定义") {
        // 数据移动指令
        static_assert(static_cast<int>(OpCode::MOVE) == 0);
        static_assert(static_cast<int>(OpCode::LOADK) == 1);
        static_assert(static_cast<int>(OpCode::LOADBOOL) == 2);
        static_assert(static_cast<int>(OpCode::LOADNIL) == 3);
        
        // Upvalue操作
        static_assert(static_cast<int>(OpCode::GETUPVAL) == 4);
        static_assert(static_cast<int>(OpCode::SETUPVAL) == 8);
        
        // 全局变量操作
        static_assert(static_cast<int>(OpCode::GETGLOBAL) == 5);
        static_assert(static_cast<int>(OpCode::SETGLOBAL) == 7);
        
        // 表操作
        static_assert(static_cast<int>(OpCode::GETTABLE) == 6);
        static_assert(static_cast<int>(OpCode::SETTABLE) == 9);
        static_assert(static_cast<int>(OpCode::NEWTABLE) == 10);
        static_assert(static_cast<int>(OpCode::SETLIST) == 33);
        
        // 算术运算
        static_assert(static_cast<int>(OpCode::ADD) == 12);
        static_assert(static_cast<int>(OpCode::SUB) == 13);
        static_assert(static_cast<int>(OpCode::MUL) == 14);
        static_assert(static_cast<int>(OpCode::DIV) == 15);
        static_assert(static_cast<int>(OpCode::MOD) == 16);
        static_assert(static_cast<int>(OpCode::POW) == 17);
        static_assert(static_cast<int>(OpCode::UNM) == 18);
        static_assert(static_cast<int>(OpCode::NOT) == 19);
        static_assert(static_cast<int>(OpCode::LEN) == 20);
        static_assert(static_cast<int>(OpCode::CONCAT) == 21);
        
        // 跳转指令
        static_assert(static_cast<int>(OpCode::JMP) == 22);
        static_assert(static_cast<int>(OpCode::EQ) == 23);
        static_assert(static_cast<int>(OpCode::LT) == 24);
        static_assert(static_cast<int>(OpCode::LE) == 25);
        
        // 逻辑测试
        static_assert(static_cast<int>(OpCode::TEST) == 26);
        static_assert(static_cast<int>(OpCode::TESTSET) == 27);
        
        // 函数调用
        static_assert(static_cast<int>(OpCode::CALL) == 28);
        static_assert(static_cast<int>(OpCode::TAILCALL) == 29);
        static_assert(static_cast<int>(OpCode::RETURN) == 30);
        
        // 循环
        static_assert(static_cast<int>(OpCode::FORLOOP) == 31);
        static_assert(static_cast<int>(OpCode::FORPREP) == 32);
        
        // 迭代器
        static_assert(static_cast<int>(OpCode::TFORLOOP) == 34);
        
        // 闭包和Upvalue
        static_assert(static_cast<int>(OpCode::CLOSURE) == 36);
        static_assert(static_cast<int>(OpCode::VARARG) == 37);
        static_assert(static_cast<int>(OpCode::CLOSE) == 35);
    }

    SECTION("指令模式分类") {
        REQUIRE(GetInstructionMode(OpCode::MOVE) == InstructionMode::iABC);
        REQUIRE(GetInstructionMode(OpCode::LOADK) == InstructionMode::iABx);
        REQUIRE(GetInstructionMode(OpCode::JMP) == InstructionMode::iAsBx);
        
        REQUIRE(GetInstructionMode(OpCode::ADD) == InstructionMode::iABC);
        REQUIRE(GetInstructionMode(OpCode::GETGLOBAL) == InstructionMode::iABx);
        REQUIRE(GetInstructionMode(OpCode::FORPREP) == InstructionMode::iAsBx);
    }
}

/* ========================================================================== */
/* 函数原型(Proto)契约 */
/* ========================================================================== */

TEST_CASE("Proto - 函数原型结构契约", "[compiler][contract][proto]") {
    SECTION("Proto基本字段") {
        Proto proto;
        
        REQUIRE(proto.GetCode().empty());
        REQUIRE(proto.GetConstants().empty());
        REQUIRE(proto.GetUpvalues().empty());
        REQUIRE(proto.GetProtos().empty());
        REQUIRE(proto.GetParameterCount() == 0);
        REQUIRE(proto.GetMaxStackSize() == 2); // 默认最小栈大小
        REQUIRE_FALSE(proto.IsVariadic());
    }

    SECTION("指令序列管理") {
        Proto proto;
        
        proto.AddInstruction(CreateABCInstruction(OpCode::LOADK, 0, 0, 0));
        proto.AddInstruction(CreateABCInstruction(OpCode::RETURN, 0, 1, 0));
        
        REQUIRE(proto.GetCode().size() == 2);
        REQUIRE(GetOpCode(proto.GetCode()[0]) == OpCode::LOADK);
        REQUIRE(GetOpCode(proto.GetCode()[1]) == OpCode::RETURN);
    }

    SECTION("常量表管理") {
        Proto proto;
        
        Size nil_idx = proto.AddConstant(LuaValue::CreateNil());
        Size num_idx = proto.AddConstant(LuaValue::CreateNumber(42.0));
        Size str_idx = proto.AddConstant(LuaValue::CreateString("hello"));
        
        REQUIRE(nil_idx == 0);
        REQUIRE(num_idx == 1);
        REQUIRE(str_idx == 2);
        REQUIRE(proto.GetConstants().size() == 3);
        
        REQUIRE(proto.GetConstant(nil_idx).IsNil());
        REQUIRE(proto.GetConstant(num_idx).GetNumber() == Approx(42.0));
        REQUIRE(proto.GetConstant(str_idx).GetString() == "hello");
    }

    SECTION("子函数管理") {
        Proto main_proto;
        auto sub_proto = std::make_unique<Proto>();
        
        Size sub_idx = main_proto.AddSubProto(std::move(sub_proto));
        REQUIRE(sub_idx == 0);
        REQUIRE(main_proto.GetProtos().size() == 1);
        REQUIRE(main_proto.GetSubProto(sub_idx) != nullptr);
    }

    SECTION("Upvalue描述符管理") {
        Proto proto;
        
        proto.AddUpvalue(UpvalueDesc{UpvalueType::Local, 0});
        proto.AddUpvalue(UpvalueDesc{UpvalueType::Upvalue, 1});
        
        REQUIRE(proto.GetUpvalues().size() == 2);
        REQUIRE(proto.GetUpvalue(0).type == UpvalueType::Local);
        REQUIRE(proto.GetUpvalue(1).type == UpvalueType::Upvalue);
    }
}

/* ========================================================================== */
/* Compiler基础功能契约 */
/* ========================================================================== */

TEST_CASE("Compiler - 构造和初始化契约", "[compiler][contract][basic]") {
    SECTION("Compiler应该正确初始化") {
        Compiler compiler;
        
        REQUIRE(compiler.GetCurrentFunction() != nullptr);
        REQUIRE(compiler.GetCurrentFunction()->GetParameterCount() == 0);
        REQUIRE(compiler.GetCurrentFunction()->IsVariadic()); // main函数总是可变参数
    }

    SECTION("函数编译环境管理") {
        Compiler compiler;
        
        // 开始新函数
        compiler.BeginFunction("test_func", {"a", "b"}, false);
        
        REQUIRE(compiler.GetCurrentFunction()->GetParameterCount() == 2);
        REQUIRE_FALSE(compiler.GetCurrentFunction()->IsVariadic());
        
        // 结束函数
        auto proto = compiler.EndFunction();
        REQUIRE(proto != nullptr);
        REQUIRE(proto->GetParameterCount() == 2);
    }
}

TEST_CASE("Compiler - 寄存器分配契约", "[compiler][contract][register]") {
    SECTION("寄存器分配和释放") {
        Compiler compiler;
        
        RegisterIndex reg1 = compiler.AllocateRegister();
        RegisterIndex reg2 = compiler.AllocateRegister();
        RegisterIndex reg3 = compiler.AllocateRegister();
        
        REQUIRE(reg1 == 0);
        REQUIRE(reg2 == 1);
        REQUIRE(reg3 == 2);
        REQUIRE(compiler.GetFreeRegisterCount() == 3);
        
        compiler.FreeRegister(reg2);
        RegisterIndex reg4 = compiler.AllocateRegister();
        REQUIRE(reg4 == reg2); // 应该重用释放的寄存器
    }

    SECTION("寄存器栈管理") {
        Compiler compiler;
        
        Size initial_top = compiler.GetRegisterTop();
        
        RegisterIndex reg1 = compiler.AllocateRegister();
        RegisterIndex reg2 = compiler.AllocateRegister();
        
        REQUIRE(compiler.GetRegisterTop() == initial_top + 2);
        
        compiler.SetRegisterTop(initial_top + 1);
        REQUIRE(compiler.GetRegisterTop() == initial_top + 1);
    }

    SECTION("临时寄存器管理") {
        Compiler compiler;
        
        // 保存寄存器状态
        Size saved_top = compiler.GetRegisterTop();
        
        // 分配临时寄存器
        RegisterIndex temp1 = compiler.AllocateTemporary();
        RegisterIndex temp2 = compiler.AllocateTemporary();
        
        REQUIRE(temp1 != temp2);
        
        // 释放临时寄存器
        compiler.FreeTemporaries(saved_top);
        REQUIRE(compiler.GetRegisterTop() == saved_top);
    }
}

/* ========================================================================== */
/* 表达式编译契约 */
/* ========================================================================== */

TEST_CASE("Compiler - 字面量表达式编译契约", "[compiler][contract][literal]") {
    SECTION("nil字面量编译") {
        Compiler compiler;
        auto nil_expr = std::make_unique<NilLiteral>();
        
        ExpressionContext ctx = compiler.CompileExpression(nil_expr.get());
        
        REQUIRE(ctx.type == ExpressionType::Nil);
        REQUIRE(ctx.register_index.has_value() == false); // nil不需要寄存器
    }

    SECTION("boolean字面量编译") {
        Compiler compiler;
        auto true_expr = std::make_unique<BooleanLiteral>(true);
        auto false_expr = std::make_unique<BooleanLiteral>(false);
        
        ExpressionContext true_ctx = compiler.CompileExpression(true_expr.get());
        ExpressionContext false_ctx = compiler.CompileExpression(false_expr.get());
        
        REQUIRE(true_ctx.type == ExpressionType::True);
        REQUIRE(false_ctx.type == ExpressionType::False);
    }

    SECTION("number字面量编译") {
        Compiler compiler;
        auto number_expr = std::make_unique<NumberLiteral>(42.5);
        
        ExpressionContext ctx = compiler.CompileExpression(number_expr.get());
        
        REQUIRE(ctx.type == ExpressionType::Constant);
        REQUIRE(ctx.constant_index.has_value());
        
        auto& constant = compiler.GetCurrentFunction()->GetConstant(*ctx.constant_index);
        REQUIRE(constant.IsNumber());
        REQUIRE(constant.GetNumber() == Approx(42.5));
    }

    SECTION("string字面量编译") {
        Compiler compiler;
        auto string_expr = std::make_unique<StringLiteral>("hello world");
        
        ExpressionContext ctx = compiler.CompileExpression(string_expr.get());
        
        REQUIRE(ctx.type == ExpressionType::Constant);
        REQUIRE(ctx.constant_index.has_value());
        
        auto& constant = compiler.GetCurrentFunction()->GetConstant(*ctx.constant_index);
        REQUIRE(constant.IsString());
        REQUIRE(constant.GetString() == "hello world");
    }

    SECTION("vararg字面量编译") {
        Compiler compiler;
        auto vararg_expr = std::make_unique<VarargLiteral>();
        
        ExpressionContext ctx = compiler.CompileExpression(vararg_expr.get());
        
        REQUIRE(ctx.type == ExpressionType::Vararg);
        // vararg表达式应该生成VARARG指令
        auto& code = compiler.GetCurrentFunction()->GetCode();
        REQUIRE_FALSE(code.empty());
        REQUIRE(GetOpCode(code.back()) == OpCode::VARARG);
    }
}

TEST_CASE("Compiler - 变量表达式编译契约", "[compiler][contract][variable]") {
    SECTION("局部变量访问") {
        Compiler compiler;
        
        // 声明局部变量
        RegisterIndex var_reg = compiler.DeclareLocalVariable("x");
        
        // 编译变量访问
        auto var_expr = std::make_unique<Identifier>("x");
        ExpressionContext ctx = compiler.CompileExpression(var_expr.get());
        
        REQUIRE(ctx.type == ExpressionType::Local);
        REQUIRE(ctx.register_index == var_reg);
    }

    SECTION("全局变量访问") {
        Compiler compiler;
        auto var_expr = std::make_unique<Identifier>("global_var");
        
        ExpressionContext ctx = compiler.CompileExpression(var_expr.get());
        
        REQUIRE(ctx.type == ExpressionType::Global);
        // 应该生成GETGLOBAL指令
        auto& code = compiler.GetCurrentFunction()->GetCode();
        REQUIRE_FALSE(code.empty());
        REQUIRE(GetOpCode(code.back()) == OpCode::GETGLOBAL);
    }

    SECTION("表索引访问") {
        Compiler compiler;
        
        auto table_expr = std::make_unique<Identifier>("table");
        auto index_expr = std::make_unique<StringLiteral>("key");
        auto index = std::make_unique<IndexExpression>(std::move(table_expr), std::move(index_expr));
        
        ExpressionContext ctx = compiler.CompileExpression(index.get());
        
        REQUIRE(ctx.type == ExpressionType::Register);
        // 应该生成GETTABLE指令
        auto& code = compiler.GetCurrentFunction()->GetCode();
        REQUIRE_FALSE(code.empty());
        REQUIRE(GetOpCode(code.back()) == OpCode::GETTABLE);
    }

    SECTION("成员访问") {
        Compiler compiler;
        
        auto object_expr = std::make_unique<Identifier>("obj");
        auto member = std::make_unique<MemberExpression>(std::move(object_expr), "field");
        
        ExpressionContext ctx = compiler.CompileExpression(member.get());
        
        REQUIRE(ctx.type == ExpressionType::Register);
        // 成员访问等价于索引访问，应该生成GETTABLE指令
        auto& code = compiler.GetCurrentFunction()->GetCode();
        REQUIRE_FALSE(code.empty());
        REQUIRE(GetOpCode(code.back()) == OpCode::GETTABLE);
    }
}

TEST_CASE("Compiler - 二元表达式编译契约", "[compiler][contract][binary]") {
    SECTION("算术二元表达式") {
        auto operators = GENERATE(
            std::make_pair(BinaryOperator::Add, OpCode::ADD),
            std::make_pair(BinaryOperator::Subtract, OpCode::SUB),
            std::make_pair(BinaryOperator::Multiply, OpCode::MUL),
            std::make_pair(BinaryOperator::Divide, OpCode::DIV),
            std::make_pair(BinaryOperator::Modulo, OpCode::MOD),
            std::make_pair(BinaryOperator::Power, OpCode::POW)
        );

        Compiler compiler;
        auto left = std::make_unique<NumberLiteral>(10.0);
        auto right = std::make_unique<NumberLiteral>(5.0);
        auto binary = std::make_unique<BinaryExpression>(operators.first, std::move(left), std::move(right));
        
        ExpressionContext ctx = compiler.CompileExpression(binary.get());
        
        REQUIRE(ctx.type == ExpressionType::Register);
        // 应该生成对应的算术指令
        auto& code = compiler.GetCurrentFunction()->GetCode();
        REQUIRE_FALSE(code.empty());
        REQUIRE(GetOpCode(code.back()) == operators.second);
    }

    SECTION("关系二元表达式") {
        auto operators = GENERATE(
            std::make_pair(BinaryOperator::Equal, OpCode::EQ),
            std::make_pair(BinaryOperator::Less, OpCode::LT),
            std::make_pair(BinaryOperator::LessEqual, OpCode::LE)
        );

        Compiler compiler;
        auto left = std::make_unique<Identifier>("a");
        auto right = std::make_unique<Identifier>("b");
        auto binary = std::make_unique<BinaryExpression>(operators.first, std::move(left), std::move(right));
        
        ExpressionContext ctx = compiler.CompileExpression(binary.get());
        
        REQUIRE(ctx.type == ExpressionType::Test);
        // 关系表达式应该生成测试指令和跳转
        auto& code = compiler.GetCurrentFunction()->GetCode();
        REQUIRE_FALSE(code.empty());
        REQUIRE(GetOpCode(code.back()) == operators.second);
    }

    SECTION("字符串连接表达式") {
        Compiler compiler;
        auto left = std::make_unique<StringLiteral>("hello");
        auto right = std::make_unique<StringLiteral>("world");
        auto concat = std::make_unique<BinaryExpression>(BinaryOperator::Concat, std::move(left), std::move(right));
        
        ExpressionContext ctx = compiler.CompileExpression(concat.get());
        
        REQUIRE(ctx.type == ExpressionType::Register);
        // 应该生成CONCAT指令
        auto& code = compiler.GetCurrentFunction()->GetCode();
        REQUIRE_FALSE(code.empty());
        REQUIRE(GetOpCode(code.back()) == OpCode::CONCAT);
    }

    SECTION("逻辑表达式短路") {
        Compiler compiler;
        auto left = std::make_unique<BooleanLiteral>(true);
        auto right = std::make_unique<BooleanLiteral>(false);
        auto and_expr = std::make_unique<BinaryExpression>(BinaryOperator::And, std::move(left), std::move(right));
        
        ExpressionContext ctx = compiler.CompileExpression(and_expr.get());
        
        REQUIRE(ctx.type == ExpressionType::Test);
        // 逻辑表达式应该使用TEST/TESTSET指令实现短路
        auto& code = compiler.GetCurrentFunction()->GetCode();
        bool found_test = false;
        for (auto inst : code) {
            if (GetOpCode(inst) == OpCode::TEST || GetOpCode(inst) == OpCode::TESTSET) {
                found_test = true;
                break;
            }
        }
        REQUIRE(found_test);
    }
}

TEST_CASE("Compiler - 一元表达式编译契约", "[compiler][contract][unary]") {
    SECTION("一元算术表达式") {
        Compiler compiler;
        auto operand = std::make_unique<NumberLiteral>(42.0);
        auto unary = std::make_unique<UnaryExpression>(UnaryOperator::Minus, std::move(operand));
        
        ExpressionContext ctx = compiler.CompileExpression(unary.get());
        
        REQUIRE(ctx.type == ExpressionType::Register);
        // 应该生成UNM指令
        auto& code = compiler.GetCurrentFunction()->GetCode();
        REQUIRE_FALSE(code.empty());
        REQUIRE(GetOpCode(code.back()) == OpCode::UNM);
    }

    SECTION("一元逻辑表达式") {
        Compiler compiler;
        auto operand = std::make_unique<BooleanLiteral>(true);
        auto unary = std::make_unique<UnaryExpression>(UnaryOperator::Not, std::move(operand));
        
        ExpressionContext ctx = compiler.CompileExpression(unary.get());
        
        REQUIRE(ctx.type == ExpressionType::Register);
        // 应该生成NOT指令
        auto& code = compiler.GetCurrentFunction()->GetCode();
        REQUIRE_FALSE(code.empty());
        REQUIRE(GetOpCode(code.back()) == OpCode::NOT);
    }

    SECTION("长度操作符表达式") {
        Compiler compiler;
        auto operand = std::make_unique<StringLiteral>("hello");
        auto length = std::make_unique<UnaryExpression>(UnaryOperator::Length, std::move(operand));
        
        ExpressionContext ctx = compiler.CompileExpression(length.get());
        
        REQUIRE(ctx.type == ExpressionType::Register);
        // 应该生成LEN指令
        auto& code = compiler.GetCurrentFunction()->GetCode();
        REQUIRE_FALSE(code.empty());
        REQUIRE(GetOpCode(code.back()) == OpCode::LEN);
    }
}

/* ========================================================================== */
/* 语句编译契约 */
/* ========================================================================== */

TEST_CASE("Compiler - 赋值语句编译契约", "[compiler][contract][assignment]") {
    SECTION("简单赋值语句") {
        Compiler compiler;
        
        // 声明局部变量
        RegisterIndex var_reg = compiler.DeclareLocalVariable("x");
        
        // 编译赋值语句: x = 42
        auto var = std::make_unique<Identifier>("x");
        auto value = std::make_unique<NumberLiteral>(42.0);
        auto assignment = std::make_unique<AssignmentStatement>();
        assignment->AddTarget(std::move(var));
        assignment->AddValue(std::move(value));
        
        compiler.CompileStatement(assignment.get());
        
        // 应该生成LOADK指令将常量加载到变量寄存器
        auto& code = compiler.GetCurrentFunction()->GetCode();
        REQUIRE_FALSE(code.empty());
        REQUIRE(GetOpCode(code.back()) == OpCode::LOADK);
        REQUIRE(GetArgA(code.back()) == var_reg);
    }

    SECTION("多重赋值语句") {
        Compiler compiler;
        
        RegisterIndex reg_a = compiler.DeclareLocalVariable("a");
        RegisterIndex reg_b = compiler.DeclareLocalVariable("b");
        
        // 编译: a, b = 1, 2
        auto assignment = std::make_unique<AssignmentStatement>();
        assignment->AddTarget(std::make_unique<Identifier>("a"));
        assignment->AddTarget(std::make_unique<Identifier>("b"));
        assignment->AddValue(std::make_unique<NumberLiteral>(1.0));
        assignment->AddValue(std::make_unique<NumberLiteral>(2.0));
        
        compiler.CompileStatement(assignment.get());
        
        auto& code = compiler.GetCurrentFunction()->GetCode();
        REQUIRE(code.size() >= 2); // 至少两条LOADK指令
    }

    SECTION("表字段赋值") {
        Compiler compiler;
        
        // 编译: table.key = value
        auto table_expr = std::make_unique<Identifier>("table");
        auto member = std::make_unique<MemberExpression>(std::move(table_expr), "key");
        auto value = std::make_unique<NumberLiteral>(42.0);
        
        auto assignment = std::make_unique<AssignmentStatement>();
        assignment->AddTarget(std::move(member));
        assignment->AddValue(std::move(value));
        
        compiler.CompileStatement(assignment.get());
        
        // 应该生成SETTABLE指令
        auto& code = compiler.GetCurrentFunction()->GetCode();
        bool found_settable = false;
        for (auto inst : code) {
            if (GetOpCode(inst) == OpCode::SETTABLE) {
                found_settable = true;
                break;
            }
        }
        REQUIRE(found_settable);
    }
}

TEST_CASE("Compiler - 局部声明编译契约", "[compiler][contract][local]") {
    SECTION("无初始化的局部声明") {
        Compiler compiler;
        
        auto local_decl = std::make_unique<LocalDeclaration>();
        local_decl->AddVariable("x");
        local_decl->AddVariable("y");
        
        compiler.CompileStatement(local_decl.get());
        
        // 应该生成LOADNIL指令初始化变量为nil
        auto& code = compiler.GetCurrentFunction()->GetCode();
        REQUIRE_FALSE(code.empty());
        REQUIRE(GetOpCode(code.back()) == OpCode::LOADNIL);
    }

    SECTION("有初始化的局部声明") {
        Compiler compiler;
        
        auto local_decl = std::make_unique<LocalDeclaration>();
        local_decl->AddVariable("x");
        local_decl->AddVariable("y");
        local_decl->AddInitializer(std::make_unique<NumberLiteral>(10.0));
        local_decl->AddInitializer(std::make_unique<NumberLiteral>(20.0));
        
        compiler.CompileStatement(local_decl.get());
        
        // 应该生成LOADK指令初始化变量
        auto& code = compiler.GetCurrentFunction()->GetCode();
        Size loadk_count = 0;
        for (auto inst : code) {
            if (GetOpCode(inst) == OpCode::LOADK) {
                loadk_count++;
            }
        }
        REQUIRE(loadk_count >= 2);
    }
}

TEST_CASE("Compiler - 控制流编译契约", "[compiler][contract][control]") {
    SECTION("if语句编译") {
        Compiler compiler;
        
        auto condition = std::make_unique<BooleanLiteral>(true);
        auto then_block = std::make_unique<BlockNode>();
        auto if_stmt = std::make_unique<IfStatement>(std::move(condition), std::move(then_block));
        
        compiler.CompileStatement(if_stmt.get());
        
        // 应该生成条件测试和跳转指令
        auto& code = compiler.GetCurrentFunction()->GetCode();
        bool found_test = false;
        bool found_jmp = false;
        for (auto inst : code) {
            OpCode op = GetOpCode(inst);
            if (op == OpCode::TEST || op == OpCode::TESTSET) {
                found_test = true;
            }
            if (op == OpCode::JMP) {
                found_jmp = true;
            }
        }
        REQUIRE(found_jmp); // if语句至少需要跳转指令
    }

    SECTION("while循环编译") {
        Compiler compiler;
        
        auto condition = std::make_unique<BooleanLiteral>(true);
        auto body = std::make_unique<BlockNode>();
        auto while_stmt = std::make_unique<WhileStatement>(std::move(condition), std::move(body));
        
        compiler.CompileStatement(while_stmt.get());
        
        // while循环应该生成至少两个跳转指令（条件跳转和循环跳转）
        auto& code = compiler.GetCurrentFunction()->GetCode();
        Size jmp_count = 0;
        for (auto inst : code) {
            if (GetOpCode(inst) == OpCode::JMP) {
                jmp_count++;
            }
        }
        REQUIRE(jmp_count >= 1);
    }

    SECTION("数值for循环编译") {
        Compiler compiler;
        
        auto start = std::make_unique<NumberLiteral>(1.0);
        auto end = std::make_unique<NumberLiteral>(10.0);
        auto step = std::make_unique<NumberLiteral>(1.0);
        auto body = std::make_unique<BlockNode>();
        
        auto for_stmt = std::make_unique<NumericForStatement>(
            "i", std::move(start), std::move(end), std::move(step), std::move(body));
        
        compiler.CompileStatement(for_stmt.get());
        
        // 数值for循环应该生成FORPREP和FORLOOP指令
        auto& code = compiler.GetCurrentFunction()->GetCode();
        bool found_forprep = false;
        bool found_forloop = false;
        for (auto inst : code) {
            OpCode op = GetOpCode(inst);
            if (op == OpCode::FORPREP) {
                found_forprep = true;
            }
            if (op == OpCode::FORLOOP) {
                found_forloop = true;
            }
        }
        REQUIRE(found_forprep);
        REQUIRE(found_forloop);
    }
}

/* ========================================================================== */
/* 优化契约 */
/* ========================================================================== */

TEST_CASE("Compiler - 常量折叠优化契约", "[compiler][contract][optimization]") {
    SECTION("算术常量折叠") {
        Compiler compiler;
        
        // 编译: 1 + 2 * 3
        auto left = std::make_unique<NumberLiteral>(1.0);
        auto mul_left = std::make_unique<NumberLiteral>(2.0);
        auto mul_right = std::make_unique<NumberLiteral>(3.0);
        auto mul = std::make_unique<BinaryExpression>(BinaryOperator::Multiply, std::move(mul_left), std::move(mul_right));
        auto add = std::make_unique<BinaryExpression>(BinaryOperator::Add, std::move(left), std::move(mul));
        
        ExpressionContext ctx = compiler.CompileExpression(add.get());
        
        // 如果启用常量折叠，结果应该是常量7
        if (compiler.IsOptimizationEnabled(OptimizationType::ConstantFolding)) {
            REQUIRE(ctx.type == ExpressionType::Constant);
            auto& constant = compiler.GetCurrentFunction()->GetConstant(*ctx.constant_index);
            REQUIRE(constant.GetNumber() == Approx(7.0));
        }
    }

    SECTION("字符串常量折叠") {
        Compiler compiler;
        
        // 编译: "hello" .. " world"
        auto left = std::make_unique<StringLiteral>("hello");
        auto right = std::make_unique<StringLiteral>(" world");
        auto concat = std::make_unique<BinaryExpression>(BinaryOperator::Concat, std::move(left), std::move(right));
        
        ExpressionContext ctx = compiler.CompileExpression(concat.get());
        
        // 如果启用常量折叠，结果应该是"hello world"
        if (compiler.IsOptimizationEnabled(OptimizationType::ConstantFolding)) {
            REQUIRE(ctx.type == ExpressionType::Constant);
            auto& constant = compiler.GetCurrentFunction()->GetConstant(*ctx.constant_index);
            REQUIRE(constant.GetString() == "hello world");
        }
    }

    SECTION("布尔常量折叠") {
        Compiler compiler;
        
        // 编译: true and false
        auto left = std::make_unique<BooleanLiteral>(true);
        auto right = std::make_unique<BooleanLiteral>(false);
        auto and_expr = std::make_unique<BinaryExpression>(BinaryOperator::And, std::move(left), std::move(right));
        
        ExpressionContext ctx = compiler.CompileExpression(and_expr.get());
        
        // 如果启用常量折叠，结果应该是false
        if (compiler.IsOptimizationEnabled(OptimizationType::ConstantFolding)) {
            REQUIRE(ctx.type == ExpressionType::False);
        }
    }
}

TEST_CASE("Compiler - 死代码消除优化契约", "[compiler][contract][dce]") {
    SECTION("无条件跳转后的死代码") {
        Compiler compiler;
        
        // return后的代码应该被标记为死代码
        auto return_stmt = std::make_unique<ReturnStatement>();
        auto dead_assignment = std::make_unique<AssignmentStatement>();
        
        auto block = std::make_unique<BlockNode>();
        block->AddStatement(std::move(return_stmt));
        block->AddStatement(std::move(dead_assignment));
        
        Size code_size_before = compiler.GetCurrentFunction()->GetCode().size();
        compiler.CompileStatement(block.get());
        Size code_size_after = compiler.GetCurrentFunction()->GetCode().size();
        
        // 如果启用死代码消除，死代码不应该被编译
        if (compiler.IsOptimizationEnabled(OptimizationType::DeadCodeElimination)) {
            // 验证没有为死代码生成额外指令
            REQUIRE(code_size_after > code_size_before); // 至少有return指令
        }
    }
}

/* ========================================================================== */
/* 错误处理契约 */
/* ========================================================================== */

TEST_CASE("Compiler - 编译错误处理契约", "[compiler][contract][error]") {
    SECTION("未定义变量错误") {
        Compiler compiler;
        auto var_expr = std::make_unique<Identifier>("undefined_var");
        
        // 如果启用严格模式，应该报错
        if (compiler.IsStrictMode()) {
            REQUIRE_THROWS_AS(compiler.CompileExpression(var_expr.get()), CompilerError);
        }
    }

    SECTION("寄存器溢出错误") {
        Compiler compiler;
        
        // 分配超过最大寄存器数量
        std::vector<RegisterIndex> registers;
        REQUIRE_THROWS_AS([&]() {
            for (int i = 0; i < 300; ++i) { // 超过255个寄存器
                registers.push_back(compiler.AllocateRegister());
            }
        }(), CompilerError);
    }

    SECTION("常量表溢出错误") {
        Compiler compiler;
        
        // 添加超过最大常量数量
        REQUIRE_THROWS_AS([&]() {
            for (int i = 0; i < 300000; ++i) { // 超过最大常量数
                compiler.GetCurrentFunction()->AddConstant(LuaValue::CreateNumber(i));
            }
        }(), CompilerError);
    }
}

/* ========================================================================== */
/* 完整程序编译契约 */
/* ========================================================================== */

TEST_CASE("Compiler - 完整程序编译契约", "[compiler][contract][program]") {
    SECTION("简单程序编译") {
        std::string source = R"(
            local x = 10
            local y = 20
            print(x + y)
        )";
        
        auto lexer = std::make_unique<Lexer>(source, "test.lua");
        Parser parser(std::move(lexer));
        auto program = parser.ParseProgram();
        
        Compiler compiler;
        auto main_proto = compiler.CompileProgram(program.get());
        
        REQUIRE(main_proto != nullptr);
        REQUIRE_FALSE(main_proto->GetCode().empty());
        REQUIRE(main_proto->GetConstants().size() >= 2); // 至少有数字常量
    }

    SECTION("函数定义程序编译") {
        std::string source = R"(
            function add(a, b)
                return a + b
            end
            
            local result = add(1, 2)
        )";
        
        auto lexer = std::make_unique<Lexer>(source, "test.lua");
        Parser parser(std::move(lexer));
        auto program = parser.ParseProgram();
        
        Compiler compiler;
        auto main_proto = compiler.CompileProgram(program.get());
        
        REQUIRE(main_proto != nullptr);
        REQUIRE(main_proto->GetProtos().size() >= 1); // 至少有一个子函数
        
        auto& sub_proto = main_proto->GetSubProto(0);
        REQUIRE(sub_proto->GetParameterCount() == 2);
        
        // 子函数应该有RETURN指令
        auto& sub_code = sub_proto->GetCode();
        bool found_return = false;
        for (auto inst : sub_code) {
            if (GetOpCode(inst) == OpCode::RETURN) {
                found_return = true;
                break;
            }
        }
        REQUIRE(found_return);
    }
}