/**
 * @file test_compiler_integration.cpp
 * @brief 编译器集成测试
 * @description 测试编译器各组件协作和端到端编译功能
 * @date 2025-09-25
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

// 编译器组件
#include "compiler/compiler.h"
#include "compiler/bytecode_generator.h"
#include "compiler/constant_pool.h"
#include "compiler/register_allocator.h"

// AST组件（假设存在）
#include "ast/ast_nodes.h"
#include "ast/expression_nodes.h"
#include "ast/statement_nodes.h"

// 核心组件
#include "core/lua_value.h"
#include "core/lua_errors.h"

using namespace lua_cpp;
using Catch::Approx;

/* ========================================================================== */
/* 编译器集成测试 */
/* ========================================================================== */

TEST_CASE("Compiler Integration - 基本表达式编译", "[compiler][integration]") {
    Compiler compiler;
    OptimizationConfig config;
    config.enable_constant_folding = true;
    
    SECTION("数值常量") {
        // 创建数值字面量AST节点
        auto number_expr = std::make_unique<NumberExpression>(42.0);
        
        CompilerResult result = compiler.CompileExpression(*number_expr, config);
        
        REQUIRE(result.IsSuccess());
        const Proto& proto = result.GetProto();
        
        // 验证字节码：应该是LOADK指令
        REQUIRE(proto.code.size() >= 1);
        Instruction inst = proto.code[0];
        REQUIRE(GetOpCode(inst) == OpCode::LOADK);
        
        // 验证常量池
        REQUIRE(proto.constants.size() == 1);
        REQUIRE(proto.constants[0].AsNumber() == Approx(42.0));
    }
    
    SECTION("字符串常量") {
        auto string_expr = std::make_unique<StringExpression>("hello world");
        
        CompilerResult result = compiler.CompileExpression(*string_expr, config);
        
        REQUIRE(result.IsSuccess());
        const Proto& proto = result.GetProto();
        
        Instruction inst = proto.code[0];
        REQUIRE(GetOpCode(inst) == OpCode::LOADK);
        REQUIRE(proto.constants[0].AsString() == "hello world");
    }
    
    SECTION("布尔常量") {
        auto true_expr = std::make_unique<BooleanExpression>(true);
        
        CompilerResult result = compiler.CompileExpression(*true_expr, config);
        
        REQUIRE(result.IsSuccess());
        const Proto& proto = result.GetProto();
        
        Instruction inst = proto.code[0];
        REQUIRE(GetOpCode(inst) == OpCode::LOADBOOL);
        REQUIRE(GetArgB(inst) == 1); // true
    }
}

TEST_CASE("Compiler Integration - 二元表达式", "[compiler][integration]") {
    Compiler compiler;
    OptimizationConfig config;
    
    SECTION("算术运算") {
        // 创建 2 + 3 表达式
        auto left = std::make_unique<NumberExpression>(2.0);
        auto right = std::make_unique<NumberExpression>(3.0);
        auto add_expr = std::make_unique<BinaryExpression>(
            BinaryOpType::Add, std::move(left), std::move(right)
        );
        
        CompilerResult result = compiler.CompileExpression(*add_expr, config);
        
        REQUIRE(result.IsSuccess());
        const Proto& proto = result.GetProto();
        
        // 验证生成的字节码序列
        REQUIRE(proto.code.size() >= 3);
        
        // LOADK R0, K0 (加载2.0)
        REQUIRE(GetOpCode(proto.code[0]) == OpCode::LOADK);
        // LOADK R1, K1 (加载3.0)
        REQUIRE(GetOpCode(proto.code[1]) == OpCode::LOADK);
        // ADD R2, R0, R1 (执行加法)
        REQUIRE(GetOpCode(proto.code[2]) == OpCode::ADD);
        
        REQUIRE(proto.constants.size() == 2);
        REQUIRE(proto.constants[0].AsNumber() == Approx(2.0));
        REQUIRE(proto.constants[1].AsNumber() == Approx(3.0));
    }
    
    SECTION("常量折叠优化") {
        config.enable_constant_folding = true;
        
        auto left = std::make_unique<NumberExpression>(5.0);
        auto right = std::make_unique<NumberExpression>(7.0);
        auto mul_expr = std::make_unique<BinaryExpression>(
            BinaryOpType::Multiply, std::move(left), std::move(right)
        );
        
        CompilerResult result = compiler.CompileExpression(*mul_expr, config);
        
        REQUIRE(result.IsSuccess());
        const Proto& proto = result.GetProto();
        
        // 应该被优化为单个LOADK指令
        REQUIRE(proto.code.size() == 1);
        REQUIRE(GetOpCode(proto.code[0]) == OpCode::LOADK);
        
        // 常量池应该包含折叠后的结果
        REQUIRE(proto.constants.size() == 1);
        REQUIRE(proto.constants[0].AsNumber() == Approx(35.0));
    }
}

TEST_CASE("Compiler Integration - 变量访问", "[compiler][integration]") {
    Compiler compiler;
    OptimizationConfig config;
    
    SECTION("局部变量") {
        // 模拟变量声明：local x = 10
        compiler.DeclareLocal("x");
        
        auto number_expr = std::make_unique<NumberExpression>(10.0);
        auto assign_result = compiler.CompileAssignment("x", *number_expr, config);
        REQUIRE(assign_result.IsSuccess());
        
        // 模拟变量使用：返回x的值
        auto var_expr = std::make_unique<VariableExpression>("x");
        CompilerResult result = compiler.CompileExpression(*var_expr, config);
        
        REQUIRE(result.IsSuccess());
        const Proto& proto = result.GetProto();
        
        // 应该生成MOVE指令从变量寄存器移动到结果寄存器
        bool found_move = false;
        for (const auto& inst : proto.code) {
            if (GetOpCode(inst) == OpCode::MOVE) {
                found_move = true;
                break;
            }
        }
        REQUIRE(found_move);
    }
}

TEST_CASE("Compiler Integration - 函数调用", "[compiler][integration]") {
    Compiler compiler;
    OptimizationConfig config;
    
    SECTION("无参数函数调用") {
        // 模拟 func() 调用
        auto func_expr = std::make_unique<VariableExpression>("func");
        std::vector<std::unique_ptr<Expression>> args;
        auto call_expr = std::make_unique<FunctionCallExpression>(
            std::move(func_expr), std::move(args)
        );
        
        CompilerResult result = compiler.CompileExpression(*call_expr, config);
        
        REQUIRE(result.IsSuccess());
        const Proto& proto = result.GetProto();
        
        // 验证CALL指令
        bool found_call = false;
        for (const auto& inst : proto.code) {
            if (GetOpCode(inst) == OpCode::CALL) {
                // CALL R(func), 1, 2 (0个参数，1个返回值)
                REQUIRE(GetArgB(inst) == 1); // nargs + 1
                REQUIRE(GetArgC(inst) == 2); // nresults + 1
                found_call = true;
                break;
            }
        }
        REQUIRE(found_call);
    }
    
    SECTION("有参数函数调用") {
        // 模拟 func(1, "test") 调用
        auto func_expr = std::make_unique<VariableExpression>("func");
        std::vector<std::unique_ptr<Expression>> args;
        args.push_back(std::make_unique<NumberExpression>(1.0));
        args.push_back(std::make_unique<StringExpression>("test"));
        
        auto call_expr = std::make_unique<FunctionCallExpression>(
            std::move(func_expr), std::move(args)
        );
        
        CompilerResult result = compiler.CompileExpression(*call_expr, config);
        
        REQUIRE(result.IsSuccess());
        const Proto& proto = result.GetProto();
        
        // 验证参数加载和CALL指令
        bool found_call = false;
        for (const auto& inst : proto.code) {
            if (GetOpCode(inst) == OpCode::CALL) {
                // CALL R(func), 3, 2 (2个参数，1个返回值)
                REQUIRE(GetArgB(inst) == 3); // nargs + 1
                found_call = true;
                break;
            }
        }
        REQUIRE(found_call);
    }
}

/* ========================================================================== */
/* 语句编译测试 */
/* ========================================================================== */

TEST_CASE("Compiler Integration - 赋值语句", "[compiler][integration]") {
    Compiler compiler;
    OptimizationConfig config;
    
    SECTION("局部变量赋值") {
        compiler.DeclareLocal("x");
        
        auto value_expr = std::make_unique<NumberExpression>(42.0);
        CompilerResult result = compiler.CompileAssignment("x", *value_expr, config);
        
        REQUIRE(result.IsSuccess());
        const Proto& proto = result.GetProto();
        
        // 验证字节码：LOADK + 可能的MOVE
        REQUIRE(proto.code.size() >= 1);
        bool found_loadk = false;
        for (const auto& inst : proto.code) {
            if (GetOpCode(inst) == OpCode::LOADK) {
                found_loadk = true;
                break;
            }
        }
        REQUIRE(found_loadk);
    }
}

TEST_CASE("Compiler Integration - 控制流", "[compiler][integration]") {
    Compiler compiler;
    OptimizationConfig config;
    
    SECTION("if语句") {
        // 模拟 if condition then ... end
        auto condition = std::make_unique<BooleanExpression>(true);
        auto then_body = std::make_unique<BlockStatement>();
        
        // 在then分支中添加一个简单语句
        auto assign_expr = std::make_unique<NumberExpression>(1.0);
        then_body->AddStatement(std::make_unique<ExpressionStatement>(std::move(assign_expr)));
        
        auto if_stmt = std::make_unique<IfStatement>(
            std::move(condition), std::move(then_body), nullptr
        );
        
        CompilerResult result = compiler.CompileStatement(*if_stmt, config);
        
        REQUIRE(result.IsSuccess());
        const Proto& proto = result.GetProto();
        
        // 验证条件跳转指令
        bool found_jump = false;
        for (const auto& inst : proto.code) {
            OpCode op = GetOpCode(inst);
            if (op == OpCode::JMP || op == OpCode::TEST) {
                found_jump = true;
                break;
            }
        }
        REQUIRE(found_jump);
    }
}

/* ========================================================================== */
/* 完整程序编译测试 */
/* ========================================================================== */

TEST_CASE("Compiler Integration - 完整程序", "[compiler][integration]") {
    Compiler compiler;
    OptimizationConfig config;
    
    SECTION("简单程序") {
        // 模拟程序：
        // local x = 10
        // local y = 20  
        // return x + y
        
        auto program = std::make_unique<BlockStatement>();
        
        // local x = 10
        auto x_init = std::make_unique<NumberExpression>(10.0);
        auto x_decl = std::make_unique<LocalDeclarationStatement>("x", std::move(x_init));
        program->AddStatement(std::move(x_decl));
        
        // local y = 20
        auto y_init = std::make_unique<NumberExpression>(20.0);
        auto y_decl = std::make_unique<LocalDeclarationStatement>("y", std::move(y_init));
        program->AddStatement(std::move(y_decl));
        
        // return x + y
        auto x_ref = std::make_unique<VariableExpression>("x");
        auto y_ref = std::make_unique<VariableExpression>("y");
        auto add_expr = std::make_unique<BinaryExpression>(
            BinaryOpType::Add, std::move(x_ref), std::move(y_ref)
        );
        auto return_stmt = std::make_unique<ReturnStatement>(std::move(add_expr));
        program->AddStatement(std::move(return_stmt));
        
        CompilerResult result = compiler.CompileProgram(*program, config);
        
        REQUIRE(result.IsSuccess());
        const Proto& proto = result.GetProto();
        
        // 验证基本结构
        REQUIRE(proto.code.size() > 0);
        REQUIRE(proto.constants.size() >= 2); // 至少包含10.0和20.0
        
        // 验证最后有RETURN指令
        Instruction last_inst = proto.code.back();
        REQUIRE(GetOpCode(last_inst) == OpCode::RETURN);
        
        // 验证常量池包含预期值
        bool found_10 = false, found_20 = false;
        for (const auto& constant : proto.constants) {
            if (constant.IsNumber()) {
                double num = constant.AsNumber();
                if (num == Approx(10.0)) found_10 = true;
                if (num == Approx(20.0)) found_20 = true;
            }
        }
        REQUIRE(found_10);
        REQUIRE(found_20);
    }
}

/* ========================================================================== */
/* 优化测试 */
/* ========================================================================== */

TEST_CASE("Compiler Integration - 优化", "[compiler][integration]") {
    Compiler compiler;
    
    SECTION("常量折叠") {
        OptimizationConfig no_opt;
        no_opt.enable_constant_folding = false;
        
        OptimizationConfig with_opt;
        with_opt.enable_constant_folding = true;
        
        // 表达式：2 * 3 * 4
        auto expr1 = std::make_unique<NumberExpression>(2.0);
        auto expr2 = std::make_unique<NumberExpression>(3.0);
        auto expr3 = std::make_unique<NumberExpression>(4.0);
        
        auto mul1 = std::make_unique<BinaryExpression>(
            BinaryOpType::Multiply, std::move(expr1), std::move(expr2)
        );
        auto mul2 = std::make_unique<BinaryExpression>(
            BinaryOpType::Multiply, std::move(mul1), std::move(expr3)
        );
        
        // 复制表达式用于比较
        auto mul2_copy = mul2->Clone();
        
        CompilerResult no_opt_result = compiler.CompileExpression(*mul2, no_opt);
        CompilerResult opt_result = compiler.CompileExpression(*mul2_copy, with_opt);
        
        REQUIRE(no_opt_result.IsSuccess());
        REQUIRE(opt_result.IsSuccess());
        
        const Proto& no_opt_proto = no_opt_result.GetProto();
        const Proto& opt_proto = opt_result.GetProto();
        
        // 优化版本应该有更少的指令
        REQUIRE(opt_proto.code.size() <= no_opt_proto.code.size());
        
        // 优化版本应该有更少的常量（只包含最终结果24.0）
        REQUIRE(opt_proto.constants.size() <= no_opt_proto.constants.size());
    }
    
    SECTION("死代码消除") {
        OptimizationConfig config;
        config.enable_dead_code_elimination = true;
        
        // 创建包含死代码的程序
        auto program = std::make_unique<BlockStatement>();
        
        // return 42; (这后面的代码都是死代码)
        auto return_stmt = std::make_unique<ReturnStatement>(
            std::make_unique<NumberExpression>(42.0)
        );
        program->AddStatement(std::move(return_stmt));
        
        // local x = 10; (死代码)
        auto dead_stmt = std::make_unique<LocalDeclarationStatement>(
            "x", std::make_unique<NumberExpression>(10.0)
        );
        program->AddStatement(std::move(dead_stmt));
        
        CompilerResult result = compiler.CompileProgram(*program, config);
        
        REQUIRE(result.IsSuccess());
        const Proto& proto = result.GetProto();
        
        // 验证死代码被消除（只应该有LOADK和RETURN）
        Size expected_instructions = 2; // LOADK + RETURN
        REQUIRE(proto.code.size() <= expected_instructions);
    }
}

/* ========================================================================== */
/* 错误处理测试 */
/* ========================================================================== */

TEST_CASE("Compiler Integration - 错误处理", "[compiler][integration]") {
    Compiler compiler;
    OptimizationConfig config;
    
    SECTION("未定义变量") {
        // 尝试使用未定义的变量
        auto var_expr = std::make_unique<VariableExpression>("undefined_var");
        
        CompilerResult result = compiler.CompileExpression(*var_expr, config);
        
        REQUIRE_FALSE(result.IsSuccess());
        REQUIRE(result.GetError().GetType() == CompilerErrorType::UndefinedVariable);
        REQUIRE(result.GetError().GetMessage().find("undefined_var") != std::string::npos);
    }
    
    SECTION("重复变量声明") {
        // 在同一作用域声明相同名称的变量
        compiler.DeclareLocal("duplicate");
        
        auto expr1 = std::make_unique<NumberExpression>(1.0);
        auto decl1 = std::make_unique<LocalDeclarationStatement>("duplicate", std::move(expr1));
        
        CompilerResult result = compiler.CompileStatement(*decl1, config);
        
        REQUIRE_FALSE(result.IsSuccess());
        REQUIRE(result.GetError().GetType() == CompilerErrorType::DuplicateVariable);
    }
    
    SECTION("寄存器溢出") {
        // 创建超过寄存器限制的表达式
        OptimizationConfig limited_config = config;
        limited_config.max_registers = 5; // 限制为5个寄存器
        
        // 创建复杂表达式需要很多临时寄存器
        std::unique_ptr<Expression> complex_expr = std::make_unique<NumberExpression>(1.0);
        
        for (int i = 0; i < 10; ++i) {
            auto right = std::make_unique<NumberExpression>(static_cast<double>(i));
            complex_expr = std::make_unique<BinaryExpression>(
                BinaryOpType::Add, std::move(complex_expr), std::move(right)
            );
        }
        
        CompilerResult result = compiler.CompileExpression(*complex_expr, limited_config);
        
        // 可能成功（如果优化能处理）或失败（如果寄存器不足）
        if (!result.IsSuccess()) {
            REQUIRE(result.GetError().GetType() == CompilerErrorType::RegisterOverflow);
        }
    }
}

/* ========================================================================== */
/* 性能测试 */
/* ========================================================================== */

TEST_CASE("Compiler Integration - 性能", "[compiler][integration][performance]") {
    Compiler compiler;
    OptimizationConfig config;
    
    SECTION("大型表达式编译") {
        // 创建深度嵌套的表达式
        std::unique_ptr<Expression> deep_expr = std::make_unique<NumberExpression>(0.0);
        
        const int depth = 100;
        for (int i = 1; i <= depth; ++i) {
            auto right = std::make_unique<NumberExpression>(static_cast<double>(i));
            deep_expr = std::make_unique<BinaryExpression>(
                BinaryOpType::Add, std::move(deep_expr), std::move(right)
            );
        }
        
        // 测试编译时间
        auto start = std::chrono::high_resolution_clock::now();
        CompilerResult result = compiler.CompileExpression(*deep_expr, config);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        REQUIRE(result.IsSuccess());
        
        // 编译时间应该合理（这里只是示例，实际阈值需要调整）
        REQUIRE(duration.count() < 1000); // 小于1秒
        
        // 验证生成的字节码大小合理
        const Proto& proto = result.GetProto();
        REQUIRE(proto.code.size() > 0);
        REQUIRE(proto.code.size() < depth * 10); // 不应该过度膨胀
    }
}