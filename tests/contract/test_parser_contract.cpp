/**
 * @file test_parser_contract.cpp
 * @brief Parser（语法分析器）契约测试
 * @description 测试Lua语法分析器的所有行为契约，确保100% Lua 5.1.5兼容性
 *              包括AST节点类型、表达式解析、语句解析、错误处理等核心功能
 * @date 2025-09-20
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/generators/catch_generators.hpp>

// 注意：这些头文件还不存在，这是TDD方法 - 先写测试定义接口
#include "parser/ast.h"
#include "parser/parser.h"
#include "lexer/lexer.h"
#include "core/lua_common.h"
#include "core/lua_errors.h"

using namespace lua_cpp;
using Catch::Approx;

/* ========================================================================== */
/* AST节点基础契约 */
/* ========================================================================== */

TEST_CASE("ASTNode - 基础节点契约", "[parser][contract][ast][node]") {
    SECTION("AST节点应该包含类型和位置信息") {
        auto node = std::make_unique<ASTNode>(ASTNodeType::Block);
        REQUIRE(node->GetType() == ASTNodeType::Block);
        REQUIRE(node->GetPosition().line == 1);
        REQUIRE(node->GetPosition().column == 1);
    }

    SECTION("AST节点应该支持父子关系") {
        auto parent = std::make_unique<BlockNode>();
        auto child = std::make_unique<ExpressionStatement>();
        
        ASTNode* child_ptr = child.get();
        parent->AddChild(std::move(child));
        
        REQUIRE(parent->GetChildCount() == 1);
        REQUIRE(parent->GetChild(0) == child_ptr);
        REQUIRE(child_ptr->GetParent() == parent.get());
    }

    SECTION("AST节点应该支持访问者模式") {
        auto node = std::make_unique<NumberLiteral>(42.0);
        
        class TestVisitor : public ASTVisitor {
        public:
            bool visited_number = false;
            double number_value = 0.0;
            
            void Visit(NumberLiteral* node) override {
                visited_number = true;
                number_value = node->GetValue();
            }
        };
        
        TestVisitor visitor;
        node->Accept(&visitor);
        
        REQUIRE(visitor.visited_number);
        REQUIRE(visitor.number_value == Approx(42.0));
    }
}

TEST_CASE("ASTNode - 节点类型判断契约", "[parser][contract][ast][type]") {
    SECTION("表达式节点类型判断") {
        auto number = std::make_unique<NumberLiteral>(123.0);
        auto string = std::make_unique<StringLiteral>("hello");
        auto binary = std::make_unique<BinaryExpression>(BinaryOperator::Add);
        
        REQUIRE(number->IsExpression());
        REQUIRE(string->IsExpression());
        REQUIRE(binary->IsExpression());
        REQUIRE_FALSE(number->IsStatement());
    }

    SECTION("语句节点类型判断") {
        auto assignment = std::make_unique<AssignmentStatement>();
        auto if_stmt = std::make_unique<IfStatement>();
        auto while_stmt = std::make_unique<WhileStatement>();
        
        REQUIRE(assignment->IsStatement());
        REQUIRE(if_stmt->IsStatement());
        REQUIRE(while_stmt->IsStatement());
        REQUIRE_FALSE(assignment->IsExpression());
    }

    SECTION("字面量节点类型判断") {
        auto nil_literal = std::make_unique<NilLiteral>();
        auto bool_literal = std::make_unique<BooleanLiteral>(true);
        auto number_literal = std::make_unique<NumberLiteral>(3.14);
        auto string_literal = std::make_unique<StringLiteral>("test");
        
        REQUIRE(nil_literal->IsLiteral());
        REQUIRE(bool_literal->IsLiteral());
        REQUIRE(number_literal->IsLiteral());
        REQUIRE(string_literal->IsLiteral());
    }
}

/* ========================================================================== */
/* 表达式AST节点契约 */
/* ========================================================================== */

TEST_CASE("Expression - 字面量表达式契约", "[parser][contract][ast][literal]") {
    SECTION("nil字面量") {
        auto nil_node = std::make_unique<NilLiteral>();
        REQUIRE(nil_node->GetType() == ASTNodeType::NilLiteral);
        REQUIRE(nil_node->IsLiteral());
        REQUIRE(nil_node->IsExpression());
    }

    SECTION("boolean字面量") {
        auto true_node = std::make_unique<BooleanLiteral>(true);
        auto false_node = std::make_unique<BooleanLiteral>(false);
        
        REQUIRE(true_node->GetValue() == true);
        REQUIRE(false_node->GetValue() == false);
        REQUIRE(true_node->GetType() == ASTNodeType::BooleanLiteral);
    }

    SECTION("number字面量") {
        auto int_node = std::make_unique<NumberLiteral>(42.0);
        auto float_node = std::make_unique<NumberLiteral>(3.14159);
        auto exp_node = std::make_unique<NumberLiteral>(1.5e10);
        
        REQUIRE(int_node->GetValue() == Approx(42.0));
        REQUIRE(float_node->GetValue() == Approx(3.14159));
        REQUIRE(exp_node->GetValue() == Approx(1.5e10));
    }

    SECTION("string字面量") {
        auto empty_string = std::make_unique<StringLiteral>("");
        auto simple_string = std::make_unique<StringLiteral>("hello");
        auto escaped_string = std::make_unique<StringLiteral>("hello\\nworld");
        
        REQUIRE(empty_string->GetValue() == "");
        REQUIRE(simple_string->GetValue() == "hello");
        REQUIRE(escaped_string->GetValue() == "hello\\nworld");
    }

    SECTION("可变参数字面量") {
        auto vararg = std::make_unique<VarargLiteral>();
        REQUIRE(vararg->GetType() == ASTNodeType::VarargLiteral);
        REQUIRE(vararg->IsExpression());
    }
}

TEST_CASE("Expression - 变量表达式契约", "[parser][contract][ast][variable]") {
    SECTION("标识符表达式") {
        auto identifier = std::make_unique<Identifier>("variable_name");
        REQUIRE(identifier->GetName() == "variable_name");
        REQUIRE(identifier->GetType() == ASTNodeType::Identifier);
        REQUIRE(identifier->IsExpression());
    }

    SECTION("索引表达式") {
        auto table_expr = std::make_unique<Identifier>("table");
        auto index_expr = std::make_unique<StringLiteral>("key");
        auto index = std::make_unique<IndexExpression>(std::move(table_expr), std::move(index_expr));
        
        REQUIRE(index->GetType() == ASTNodeType::IndexExpression);
        REQUIRE(index->GetTableExpression() != nullptr);
        REQUIRE(index->GetIndexExpression() != nullptr);
    }

    SECTION("成员访问表达式") {
        auto object_expr = std::make_unique<Identifier>("object");
        auto member = std::make_unique<MemberExpression>(std::move(object_expr), "method");
        
        REQUIRE(member->GetType() == ASTNodeType::MemberExpression);
        REQUIRE(member->GetMemberName() == "method");
        REQUIRE(member->GetObjectExpression() != nullptr);
    }
}

TEST_CASE("Expression - 二元表达式契约", "[parser][contract][ast][binary]") {
    SECTION("算术二元表达式") {
        auto operators = GENERATE(
            BinaryOperator::Add,      // +
            BinaryOperator::Subtract, // -
            BinaryOperator::Multiply, // *
            BinaryOperator::Divide,   // /
            BinaryOperator::Modulo,   // %
            BinaryOperator::Power     // ^
        );

        auto left = std::make_unique<NumberLiteral>(10.0);
        auto right = std::make_unique<NumberLiteral>(5.0);
        auto binary = std::make_unique<BinaryExpression>(operators, std::move(left), std::move(right));
        
        REQUIRE(binary->GetType() == ASTNodeType::BinaryExpression);
        REQUIRE(binary->GetOperator() == operators);
        REQUIRE(binary->GetLeftOperand() != nullptr);
        REQUIRE(binary->GetRightOperand() != nullptr);
    }

    SECTION("关系二元表达式") {
        auto operators = GENERATE(
            BinaryOperator::Equal,        // ==
            BinaryOperator::NotEqual,     // ~=
            BinaryOperator::Less,         // <
            BinaryOperator::LessEqual,    // <=
            BinaryOperator::Greater,      // >
            BinaryOperator::GreaterEqual  // >=
        );

        auto left = std::make_unique<Identifier>("a");
        auto right = std::make_unique<Identifier>("b");
        auto binary = std::make_unique<BinaryExpression>(operators, std::move(left), std::move(right));
        
        REQUIRE(binary->GetOperator() == operators);
        REQUIRE(IsRelationalOperator(operators));
    }

    SECTION("逻辑二元表达式") {
        auto operators = GENERATE(
            BinaryOperator::And,     // and
            BinaryOperator::Or       // or
        );

        auto left = std::make_unique<BooleanLiteral>(true);
        auto right = std::make_unique<BooleanLiteral>(false);
        auto binary = std::make_unique<BinaryExpression>(operators, std::move(left), std::move(right));
        
        REQUIRE(binary->GetOperator() == operators);
        REQUIRE(IsLogicalOperator(operators));
    }

    SECTION("字符串连接表达式") {
        auto left = std::make_unique<StringLiteral>("hello");
        auto right = std::make_unique<StringLiteral>("world");
        auto concat = std::make_unique<BinaryExpression>(BinaryOperator::Concat, std::move(left), std::move(right));
        
        REQUIRE(concat->GetOperator() == BinaryOperator::Concat);
    }
}

TEST_CASE("Expression - 一元表达式契约", "[parser][contract][ast][unary]") {
    SECTION("一元算术表达式") {
        auto operand = std::make_unique<NumberLiteral>(42.0);
        auto unary_minus = std::make_unique<UnaryExpression>(UnaryOperator::Minus, std::move(operand));
        
        REQUIRE(unary_minus->GetType() == ASTNodeType::UnaryExpression);
        REQUIRE(unary_minus->GetOperator() == UnaryOperator::Minus);
        REQUIRE(unary_minus->GetOperand() != nullptr);
    }

    SECTION("一元逻辑表达式") {
        auto operand = std::make_unique<BooleanLiteral>(true);
        auto unary_not = std::make_unique<UnaryExpression>(UnaryOperator::Not, std::move(operand));
        
        REQUIRE(unary_not->GetOperator() == UnaryOperator::Not);
    }

    SECTION("长度操作符表达式") {
        auto operand = std::make_unique<StringLiteral>("hello");
        auto length = std::make_unique<UnaryExpression>(UnaryOperator::Length, std::move(operand));
        
        REQUIRE(length->GetOperator() == UnaryOperator::Length);
    }
}

TEST_CASE("Expression - 函数调用表达式契约", "[parser][contract][ast][call]") {
    SECTION("无参数函数调用") {
        auto function = std::make_unique<Identifier>("func");
        auto call = std::make_unique<CallExpression>(std::move(function));
        
        REQUIRE(call->GetType() == ASTNodeType::CallExpression);
        REQUIRE(call->GetFunction() != nullptr);
        REQUIRE(call->GetArgumentCount() == 0);
    }

    SECTION("有参数函数调用") {
        auto function = std::make_unique<Identifier>("print");
        auto call = std::make_unique<CallExpression>(std::move(function));
        
        call->AddArgument(std::make_unique<StringLiteral>("hello"));
        call->AddArgument(std::make_unique<NumberLiteral>(42.0));
        
        REQUIRE(call->GetArgumentCount() == 2);
        REQUIRE(call->GetArgument(0) != nullptr);
        REQUIRE(call->GetArgument(1) != nullptr);
    }

    SECTION("方法调用表达式") {
        auto object = std::make_unique<Identifier>("obj");
        auto call = std::make_unique<MethodCallExpression>(std::move(object), "method");
        
        REQUIRE(call->GetType() == ASTNodeType::MethodCallExpression);
        REQUIRE(call->GetMethodName() == "method");
        REQUIRE(call->GetObject() != nullptr);
    }
}

TEST_CASE("Expression - 表构造表达式契约", "[parser][contract][ast][table]") {
    SECTION("空表构造") {
        auto table = std::make_unique<TableConstructor>();
        REQUIRE(table->GetType() == ASTNodeType::TableConstructor);
        REQUIRE(table->GetFieldCount() == 0);
        REQUIRE(table->IsEmpty());
    }

    SECTION("数组式表构造") {
        auto table = std::make_unique<TableConstructor>();
        table->AddField(std::make_unique<NumberLiteral>(1.0));
        table->AddField(std::make_unique<NumberLiteral>(2.0));
        table->AddField(std::make_unique<NumberLiteral>(3.0));
        
        REQUIRE(table->GetFieldCount() == 3);
        REQUIRE_FALSE(table->IsEmpty());
    }

    SECTION("键值对表构造") {
        auto table = std::make_unique<TableConstructor>();
        
        auto key1 = std::make_unique<StringLiteral>("name");
        auto value1 = std::make_unique<StringLiteral>("John");
        table->AddField(std::make_unique<TableField>(std::move(key1), std::move(value1)));
        
        auto key2 = std::make_unique<StringLiteral>("age");
        auto value2 = std::make_unique<NumberLiteral>(30.0);
        table->AddField(std::make_unique<TableField>(std::move(key2), std::move(value2)));
        
        REQUIRE(table->GetFieldCount() == 2);
    }

    SECTION("混合表构造") {
        auto table = std::make_unique<TableConstructor>();
        
        // 数组部分
        table->AddField(std::make_unique<StringLiteral>("first"));
        table->AddField(std::make_unique<StringLiteral>("second"));
        
        // 键值对部分
        auto key = std::make_unique<StringLiteral>("key");
        auto value = std::make_unique<StringLiteral>("value");
        table->AddField(std::make_unique<TableField>(std::move(key), std::move(value)));
        
        REQUIRE(table->GetFieldCount() == 3);
        REQUIRE(table->HasArrayPart());
        REQUIRE(table->HasHashPart());
    }
}

/* ========================================================================== */
/* 语句AST节点契约 */
/* ========================================================================== */

TEST_CASE("Statement - 赋值语句契约", "[parser][contract][ast][assignment]") {
    SECTION("单变量赋值") {
        auto var = std::make_unique<Identifier>("x");
        auto value = std::make_unique<NumberLiteral>(42.0);
        auto assignment = std::make_unique<AssignmentStatement>();
        
        assignment->AddTarget(std::move(var));
        assignment->AddValue(std::move(value));
        
        REQUIRE(assignment->GetType() == ASTNodeType::AssignmentStatement);
        REQUIRE(assignment->GetTargetCount() == 1);
        REQUIRE(assignment->GetValueCount() == 1);
    }

    SECTION("多变量赋值") {
        auto assignment = std::make_unique<AssignmentStatement>();
        
        assignment->AddTarget(std::make_unique<Identifier>("a"));
        assignment->AddTarget(std::make_unique<Identifier>("b"));
        assignment->AddTarget(std::make_unique<Identifier>("c"));
        
        assignment->AddValue(std::make_unique<NumberLiteral>(1.0));
        assignment->AddValue(std::make_unique<NumberLiteral>(2.0));
        assignment->AddValue(std::make_unique<NumberLiteral>(3.0));
        
        REQUIRE(assignment->GetTargetCount() == 3);
        REQUIRE(assignment->GetValueCount() == 3);
    }

    SECTION("局部变量声明") {
        auto local_decl = std::make_unique<LocalDeclaration>();
        
        local_decl->AddVariable("x");
        local_decl->AddVariable("y");
        local_decl->AddInitializer(std::make_unique<NumberLiteral>(10.0));
        local_decl->AddInitializer(std::make_unique<NumberLiteral>(20.0));
        
        REQUIRE(local_decl->GetType() == ASTNodeType::LocalDeclaration);
        REQUIRE(local_decl->GetVariableCount() == 2);
        REQUIRE(local_decl->GetInitializerCount() == 2);
    }
}

TEST_CASE("Statement - 控制流语句契约", "[parser][contract][ast][control]") {
    SECTION("if语句") {
        auto condition = std::make_unique<BooleanLiteral>(true);
        auto then_block = std::make_unique<BlockNode>();
        auto if_stmt = std::make_unique<IfStatement>(std::move(condition), std::move(then_block));
        
        REQUIRE(if_stmt->GetType() == ASTNodeType::IfStatement);
        REQUIRE(if_stmt->GetCondition() != nullptr);
        REQUIRE(if_stmt->GetThenBlock() != nullptr);
        REQUIRE(if_stmt->GetElseBlock() == nullptr);
    }

    SECTION("if-else语句") {
        auto condition = std::make_unique<BooleanLiteral>(false);
        auto then_block = std::make_unique<BlockNode>();
        auto else_block = std::make_unique<BlockNode>();
        auto if_stmt = std::make_unique<IfStatement>(std::move(condition), std::move(then_block));
        if_stmt->SetElseBlock(std::move(else_block));
        
        REQUIRE(if_stmt->GetElseBlock() != nullptr);
    }

    SECTION("if-elseif-else语句") {
        auto condition1 = std::make_unique<BooleanLiteral>(false);
        auto then_block1 = std::make_unique<BlockNode>();
        auto if_stmt = std::make_unique<IfStatement>(std::move(condition1), std::move(then_block1));
        
        auto condition2 = std::make_unique<BooleanLiteral>(true);
        auto then_block2 = std::make_unique<BlockNode>();
        if_stmt->AddElseIf(std::move(condition2), std::move(then_block2));
        
        auto else_block = std::make_unique<BlockNode>();
        if_stmt->SetElseBlock(std::move(else_block));
        
        REQUIRE(if_stmt->GetElseIfCount() == 1);
        REQUIRE(if_stmt->GetElseBlock() != nullptr);
    }

    SECTION("while语句") {
        auto condition = std::make_unique<BooleanLiteral>(true);
        auto body = std::make_unique<BlockNode>();
        auto while_stmt = std::make_unique<WhileStatement>(std::move(condition), std::move(body));
        
        REQUIRE(while_stmt->GetType() == ASTNodeType::WhileStatement);
        REQUIRE(while_stmt->GetCondition() != nullptr);
        REQUIRE(while_stmt->GetBody() != nullptr);
    }

    SECTION("repeat-until语句") {
        auto body = std::make_unique<BlockNode>();
        auto condition = std::make_unique<BooleanLiteral>(false);
        auto repeat_stmt = std::make_unique<RepeatStatement>(std::move(body), std::move(condition));
        
        REQUIRE(repeat_stmt->GetType() == ASTNodeType::RepeatStatement);
        REQUIRE(repeat_stmt->GetBody() != nullptr);
        REQUIRE(repeat_stmt->GetCondition() != nullptr);
    }
}

TEST_CASE("Statement - 循环语句契约", "[parser][contract][ast][loop]") {
    SECTION("数值for循环") {
        auto var = "i";
        auto start = std::make_unique<NumberLiteral>(1.0);
        auto end = std::make_unique<NumberLiteral>(10.0);
        auto step = std::make_unique<NumberLiteral>(1.0);
        auto body = std::make_unique<BlockNode>();
        
        auto for_stmt = std::make_unique<NumericForStatement>(
            var, std::move(start), std::move(end), std::move(step), std::move(body));
        
        REQUIRE(for_stmt->GetType() == ASTNodeType::NumericForStatement);
        REQUIRE(for_stmt->GetVariable() == "i");
        REQUIRE(for_stmt->GetStart() != nullptr);
        REQUIRE(for_stmt->GetEnd() != nullptr);
        REQUIRE(for_stmt->GetStep() != nullptr);
        REQUIRE(for_stmt->GetBody() != nullptr);
    }

    SECTION("泛型for循环") {
        auto for_stmt = std::make_unique<GenericForStatement>();
        for_stmt->AddVariable("k");
        for_stmt->AddVariable("v");
        for_stmt->AddIterator(std::make_unique<Identifier>("pairs"));
        for_stmt->AddIterator(std::make_unique<Identifier>("table"));
        
        auto body = std::make_unique<BlockNode>();
        for_stmt->SetBody(std::move(body));
        
        REQUIRE(for_stmt->GetType() == ASTNodeType::GenericForStatement);
        REQUIRE(for_stmt->GetVariableCount() == 2);
        REQUIRE(for_stmt->GetIteratorCount() == 2);
        REQUIRE(for_stmt->GetBody() != nullptr);
    }

    SECTION("break语句") {
        auto break_stmt = std::make_unique<BreakStatement>();
        REQUIRE(break_stmt->GetType() == ASTNodeType::BreakStatement);
    }
}

TEST_CASE("Statement - 函数语句契约", "[parser][contract][ast][function]") {
    SECTION("函数定义语句") {
        auto func_def = std::make_unique<FunctionDefinition>("test_function");
        func_def->AddParameter("a");
        func_def->AddParameter("b");
        
        auto body = std::make_unique<BlockNode>();
        func_def->SetBody(std::move(body));
        
        REQUIRE(func_def->GetType() == ASTNodeType::FunctionDefinition);
        REQUIRE(func_def->GetName() == "test_function");
        REQUIRE(func_def->GetParameterCount() == 2);
        REQUIRE(func_def->GetBody() != nullptr);
        REQUIRE_FALSE(func_def->IsVariadic());
    }

    SECTION("可变参数函数定义") {
        auto func_def = std::make_unique<FunctionDefinition>("variadic_func");
        func_def->AddParameter("first");
        func_def->SetVariadic(true);
        
        auto body = std::make_unique<BlockNode>();
        func_def->SetBody(std::move(body));
        
        REQUIRE(func_def->IsVariadic());
        REQUIRE(func_def->GetParameterCount() == 1);
    }

    SECTION("局部函数定义") {
        auto local_func = std::make_unique<LocalFunctionDefinition>("local_func");
        auto body = std::make_unique<BlockNode>();
        local_func->SetBody(std::move(body));
        
        REQUIRE(local_func->GetType() == ASTNodeType::LocalFunctionDefinition);
        REQUIRE(local_func->GetName() == "local_func");
    }

    SECTION("return语句") {
        auto return_stmt = std::make_unique<ReturnStatement>();
        return_stmt->AddValue(std::make_unique<NumberLiteral>(42.0));
        return_stmt->AddValue(std::make_unique<StringLiteral>("done"));
        
        REQUIRE(return_stmt->GetType() == ASTNodeType::ReturnStatement);
        REQUIRE(return_stmt->GetValueCount() == 2);
    }
}

/* ========================================================================== */
/* Parser基础功能契约 */
/* ========================================================================== */

TEST_CASE("Parser - 构造和初始化契约", "[parser][contract][basic]") {
    SECTION("Parser应该正确初始化") {
        std::string source = "local x = 42";
        auto lexer = std::make_unique<Lexer>(source, "test.lua");
        Parser parser(std::move(lexer));
        
        REQUIRE_FALSE(parser.IsAtEnd());
        REQUIRE(parser.GetCurrentToken().GetType() == TokenType::Local);
    }

    SECTION("空源码解析") {
        std::string source = "";
        auto lexer = std::make_unique<Lexer>(source, "empty.lua");
        Parser parser(std::move(lexer));
        
        auto ast = parser.ParseProgram();
        REQUIRE(ast != nullptr);
        REQUIRE(ast->GetType() == ASTNodeType::Program);
        REQUIRE(ast->GetChildCount() == 0);
    }

    SECTION("只有注释的源码解析") {
        std::string source = "-- this is a comment\n--[[ multi-line comment ]]";
        auto lexer = std::make_unique<Lexer>(source, "comment.lua");
        Parser parser(std::move(lexer));
        
        auto ast = parser.ParseProgram();
        REQUIRE(ast != nullptr);
        REQUIRE(ast->GetChildCount() == 0);
    }
}

TEST_CASE("Parser - 表达式解析契约", "[parser][contract][expression]") {
    SECTION("字面量表达式解析") {
        auto testCases = GENERATE(
            std::make_pair("nil", ASTNodeType::NilLiteral),
            std::make_pair("true", ASTNodeType::BooleanLiteral),
            std::make_pair("false", ASTNodeType::BooleanLiteral),
            std::make_pair("42", ASTNodeType::NumberLiteral),
            std::make_pair("3.14", ASTNodeType::NumberLiteral),
            std::make_pair("\"hello\"", ASTNodeType::StringLiteral),
            std::make_pair("'world'", ASTNodeType::StringLiteral),
            std::make_pair("...", ASTNodeType::VarargLiteral)
        );

        std::string source = testCases.first;
        auto lexer = std::make_unique<Lexer>(source, "test.lua");
        Parser parser(std::move(lexer));
        
        auto expr = parser.ParseExpression();
        REQUIRE(expr != nullptr);
        REQUIRE(expr->GetType() == testCases.second);
    }

    SECTION("标识符表达式解析") {
        std::string source = "variable_name";
        auto lexer = std::make_unique<Lexer>(source, "test.lua");
        Parser parser(std::move(lexer));
        
        auto expr = parser.ParseExpression();
        REQUIRE(expr != nullptr);
        REQUIRE(expr->GetType() == ASTNodeType::Identifier);
        
        auto identifier = dynamic_cast<Identifier*>(expr.get());
        REQUIRE(identifier != nullptr);
        REQUIRE(identifier->GetName() == "variable_name");
    }

    SECTION("二元表达式解析") {
        auto testCases = GENERATE(
            std::make_tuple("1 + 2", BinaryOperator::Add),
            std::make_tuple("a - b", BinaryOperator::Subtract),
            std::make_tuple("x * y", BinaryOperator::Multiply),
            std::make_tuple("n / m", BinaryOperator::Divide),
            std::make_tuple("a % b", BinaryOperator::Modulo),
            std::make_tuple("x ^ y", BinaryOperator::Power),
            std::make_tuple("a .. b", BinaryOperator::Concat),
            std::make_tuple("x == y", BinaryOperator::Equal),
            std::make_tuple("a ~= b", BinaryOperator::NotEqual),
            std::make_tuple("x < y", BinaryOperator::Less),
            std::make_tuple("a <= b", BinaryOperator::LessEqual),
            std::make_tuple("x > y", BinaryOperator::Greater),
            std::make_tuple("a >= b", BinaryOperator::GreaterEqual),
            std::make_tuple("p and q", BinaryOperator::And),
            std::make_tuple("p or q", BinaryOperator::Or)
        );

        std::string source = std::get<0>(testCases);
        auto lexer = std::make_unique<Lexer>(source, "test.lua");
        Parser parser(std::move(lexer));
        
        auto expr = parser.ParseExpression();
        REQUIRE(expr != nullptr);
        REQUIRE(expr->GetType() == ASTNodeType::BinaryExpression);
        
        auto binary = dynamic_cast<BinaryExpression*>(expr.get());
        REQUIRE(binary != nullptr);
        REQUIRE(binary->GetOperator() == std::get<1>(testCases));
    }

    SECTION("一元表达式解析") {
        auto testCases = GENERATE(
            std::make_tuple("-x", UnaryOperator::Minus),
            std::make_tuple("not flag", UnaryOperator::Not),
            std::make_tuple("#table", UnaryOperator::Length)
        );

        std::string source = std::get<0>(testCases);
        auto lexer = std::make_unique<Lexer>(source, "test.lua");
        Parser parser(std::move(lexer));
        
        auto expr = parser.ParseExpression();
        REQUIRE(expr != nullptr);
        REQUIRE(expr->GetType() == ASTNodeType::UnaryExpression);
        
        auto unary = dynamic_cast<UnaryExpression*>(expr.get());
        REQUIRE(unary != nullptr);
        REQUIRE(unary->GetOperator() == std::get<1>(testCases));
    }

    SECTION("函数调用表达式解析") {
        auto testCases = GENERATE(
            "func()",
            "print('hello')",
            "math.max(a, b, c)",
            "obj:method()",
            "obj:method(arg1, arg2)"
        );

        std::string source = testCases;
        auto lexer = std::make_unique<Lexer>(source, "test.lua");
        Parser parser(std::move(lexer));
        
        auto expr = parser.ParseExpression();
        REQUIRE(expr != nullptr);
        REQUIRE((expr->GetType() == ASTNodeType::CallExpression ||
                 expr->GetType() == ASTNodeType::MethodCallExpression));
    }

    SECTION("表构造表达式解析") {
        auto testCases = GENERATE(
            "{}",
            "{1, 2, 3}",
            "{a = 1, b = 2}",
            "{[key] = value}",
            "{1, 2, a = 3, [\"key\"] = \"value\"}"
        );

        std::string source = testCases;
        auto lexer = std::make_unique<Lexer>(source, "test.lua");
        Parser parser(std::move(lexer));
        
        auto expr = parser.ParseExpression();
        REQUIRE(expr != nullptr);
        REQUIRE(expr->GetType() == ASTNodeType::TableConstructor);
    }
}

TEST_CASE("Parser - 语句解析契约", "[parser][contract][statement]") {
    SECTION("赋值语句解析") {
        auto testCases = GENERATE(
            "x = 1",
            "a, b = 1, 2",
            "table[key] = value",
            "obj.field = new_value"
        );

        std::string source = testCases;
        auto lexer = std::make_unique<Lexer>(source, "test.lua");
        Parser parser(std::move(lexer));
        
        auto stmt = parser.ParseStatement();
        REQUIRE(stmt != nullptr);
        REQUIRE(stmt->GetType() == ASTNodeType::AssignmentStatement);
    }

    SECTION("局部变量声明解析") {
        auto testCases = GENERATE(
            "local x",
            "local a, b",
            "local x = 1",
            "local a, b = 1, 2"
        );

        std::string source = testCases;
        auto lexer = std::make_unique<Lexer>(source, "test.lua");
        Parser parser(std::move(lexer));
        
        auto stmt = parser.ParseStatement();
        REQUIRE(stmt != nullptr);
        REQUIRE(stmt->GetType() == ASTNodeType::LocalDeclaration);
    }

    SECTION("函数定义解析") {
        auto testCases = GENERATE(
            "function f() end",
            "function f(a, b) end",
            "function f(a, b, ...) end",
            "local function f() end",
            "function obj:method() end"
        );

        std::string source = testCases;
        auto lexer = std::make_unique<Lexer>(source, "test.lua");
        Parser parser(std::move(lexer));
        
        auto stmt = parser.ParseStatement();
        REQUIRE(stmt != nullptr);
        REQUIRE((stmt->GetType() == ASTNodeType::FunctionDefinition ||
                 stmt->GetType() == ASTNodeType::LocalFunctionDefinition));
    }

    SECTION("控制流语句解析") {
        auto testCases = GENERATE(
            "if condition then end",
            "if a then elseif b then else end",
            "while condition do end",
            "repeat until condition",
            "for i = 1, 10 do end",
            "for k, v in pairs(table) do end",
            "break",
            "return",
            "return value",
            "return a, b, c"
        );

        std::string source = testCases;
        auto lexer = std::make_unique<Lexer>(source, "test.lua");
        Parser parser(std::move(lexer));
        
        auto stmt = parser.ParseStatement();
        REQUIRE(stmt != nullptr);
        REQUIRE(stmt->IsStatement());
    }
}

/* ========================================================================== */
/* Parser错误处理契约 */
/* ========================================================================== */

TEST_CASE("Parser - 语法错误处理契约", "[parser][contract][error]") {
    SECTION("意外的Token应该报错") {
        auto testCases = GENERATE(
            "local 123",           // 标识符位置出现数字
            "function 'name'() end", // 函数名位置出现字符串
            "if then end",         // 缺少条件
            "while do end",        // 缺少条件
            "for in do end"        // 缺少变量和迭代器
        );

        std::string source = testCases;
        auto lexer = std::make_unique<Lexer>(source, "test.lua");
        Parser parser(std::move(lexer));
        
        REQUIRE_THROWS_AS(parser.ParseStatement(), SyntaxError);
    }

    SECTION("不匹配的括号应该报错") {
        auto testCases = GENERATE(
            "func(",              // 缺少右括号
            "func)",              // 缺少左括号
            "{1, 2",              // 缺少右大括号
            "1, 2}",              // 缺少左大括号
            "[1, 2",              // 缺少右方括号
            "1, 2]"               // 缺少左方括号
        );

        std::string source = testCases;
        auto lexer = std::make_unique<Lexer>(source, "test.lua");
        Parser parser(std::move(lexer));
        
        REQUIRE_THROWS_AS(parser.ParseExpression(), SyntaxError);
    }

    SECTION("不完整的语句应该报错") {
        auto testCases = GENERATE(
            "if condition then",   // 缺少end
            "while condition do",  // 缺少end
            "function f()",        // 缺少end
            "repeat",              // 缺少until
            "for i = 1, 10 do"     // 缺少end
        );

        std::string source = testCases;
        auto lexer = std::make_unique<Lexer>(source, "test.lua");
        Parser parser(std::move(lexer));
        
        REQUIRE_THROWS_AS(parser.ParseStatement(), SyntaxError);
    }

    SECTION("语法错误应该包含位置信息") {
        std::string source = "local 123 invalid";
        auto lexer = std::make_unique<Lexer>(source, "test.lua");
        Parser parser(std::move(lexer));
        
        try {
            parser.ParseStatement();
            FAIL("应该抛出语法错误");
        } catch (const SyntaxError& e) {
            REQUIRE(e.GetPosition().line == 1);
            REQUIRE(e.GetPosition().column > 1);
            REQUIRE(e.GetPosition().source == "test.lua");
        }
    }
}

/* ========================================================================== */
/* Parser优先级和结合性契约 */
/* ========================================================================== */

TEST_CASE("Parser - 操作符优先级契约", "[parser][contract][precedence]") {
    SECTION("算术操作符优先级") {
        std::string source = "1 + 2 * 3";
        auto lexer = std::make_unique<Lexer>(source, "test.lua");
        Parser parser(std::move(lexer));
        
        auto expr = parser.ParseExpression();
        REQUIRE(expr != nullptr);
        REQUIRE(expr->GetType() == ASTNodeType::BinaryExpression);
        
        auto binary = dynamic_cast<BinaryExpression*>(expr.get());
        REQUIRE(binary->GetOperator() == BinaryOperator::Add);
        
        // 右操作数应该是乘法表达式
        auto right = binary->GetRightOperand();
        REQUIRE(right->GetType() == ASTNodeType::BinaryExpression);
        
        auto right_binary = dynamic_cast<BinaryExpression*>(right);
        REQUIRE(right_binary->GetOperator() == BinaryOperator::Multiply);
    }

    SECTION("关系操作符与逻辑操作符优先级") {
        std::string source = "a < b and c > d";
        auto lexer = std::make_unique<Lexer>(source, "test.lua");
        Parser parser(std::move(lexer));
        
        auto expr = parser.ParseExpression();
        REQUIRE(expr != nullptr);
        REQUIRE(expr->GetType() == ASTNodeType::BinaryExpression);
        
        auto binary = dynamic_cast<BinaryExpression*>(expr.get());
        REQUIRE(binary->GetOperator() == BinaryOperator::And);
        
        // 左右操作数都应该是关系表达式
        REQUIRE(binary->GetLeftOperand()->GetType() == ASTNodeType::BinaryExpression);
        REQUIRE(binary->GetRightOperand()->GetType() == ASTNodeType::BinaryExpression);
    }

    SECTION("右结合性操作符") {
        std::string source = "a ^ b ^ c";
        auto lexer = std::make_unique<Lexer>(source, "test.lua");
        Parser parser(std::move(lexer));
        
        auto expr = parser.ParseExpression();
        REQUIRE(expr != nullptr);
        REQUIRE(expr->GetType() == ASTNodeType::BinaryExpression);
        
        auto binary = dynamic_cast<BinaryExpression*>(expr.get());
        REQUIRE(binary->GetOperator() == BinaryOperator::Power);
        
        // 应该解析为 a ^ (b ^ c)
        auto right = binary->GetRightOperand();
        REQUIRE(right->GetType() == ASTNodeType::BinaryExpression);
    }

    SECTION("字符串连接右结合性") {
        std::string source = "\"a\" .. \"b\" .. \"c\"";
        auto lexer = std::make_unique<Lexer>(source, "test.lua");
        Parser parser(std::move(lexer));
        
        auto expr = parser.ParseExpression();
        REQUIRE(expr != nullptr);
        REQUIRE(expr->GetType() == ASTNodeType::BinaryExpression);
        
        auto binary = dynamic_cast<BinaryExpression*>(expr.get());
        REQUIRE(binary->GetOperator() == BinaryOperator::Concat);
        
        // 应该解析为 "a" .. ("b" .. "c")
        auto right = binary->GetRightOperand();
        REQUIRE(right->GetType() == ASTNodeType::BinaryExpression);
    }
}

/* ========================================================================== */
/* Parser完整程序解析契约 */
/* ========================================================================== */

TEST_CASE("Parser - 完整程序解析契约", "[parser][contract][program]") {
    SECTION("简单程序解析") {
        std::string source = R"(
            local x = 10
            local y = 20
            print(x + y)
        )";
        
        auto lexer = std::make_unique<Lexer>(source, "test.lua");
        Parser parser(std::move(lexer));
        
        auto program = parser.ParseProgram();
        REQUIRE(program != nullptr);
        REQUIRE(program->GetType() == ASTNodeType::Program);
        REQUIRE(program->GetChildCount() == 3); // 两个局部声明 + 一个函数调用
    }

    SECTION("函数定义程序解析") {
        std::string source = R"(
            function factorial(n)
                if n <= 1 then
                    return 1
                else
                    return n * factorial(n - 1)
                end
            end
            
            print(factorial(5))
        )";
        
        auto lexer = std::make_unique<Lexer>(source, "test.lua");
        Parser parser(std::move(lexer));
        
        auto program = parser.ParseProgram();
        REQUIRE(program != nullptr);
        REQUIRE(program->GetChildCount() == 2); // 函数定义 + 函数调用
    }

    SECTION("复杂程序解析") {
        std::string source = R"(
            -- 局部变量和表
            local data = {
                name = "test",
                values = {1, 2, 3, 4, 5}
            }
            
            -- 函数定义
            function process(table)
                local sum = 0
                for i, v in ipairs(table.values) do
                    sum = sum + v
                end
                return sum
            end
            
            -- 控制流
            local result = process(data)
            if result > 10 then
                print("Large sum: " .. result)
            else
                print("Small sum: " .. result)
            end
        )";
        
        auto lexer = std::make_unique<Lexer>(source, "test.lua");
        Parser parser(std::move(lexer));
        
        auto program = parser.ParseProgram();
        REQUIRE(program != nullptr);
        REQUIRE(program->GetChildCount() >= 3); // 至少有局部声明、函数定义、if语句
    }
}