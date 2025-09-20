/**
 * @file test_parser_contract.cpp
 * @brief Lua语法分析器契约测试
 * @description 基于lua_c_analysis和lua_with_cpp双重验证的Parser契约测试
 * @date 2025-09-20
 * 
 * 测试覆盖范围：
 * 1. 基础语法结构解析
 * 2. 表达式解析和优先级
 * 3. 语句解析和控制流
 * 4. 函数定义和调用
 * 5. 表构造和访问
 * 6. 错误检测和恢复
 * 7. AST构建验证
 * 8. 边界条件处理
 * 
 * 双重验证机制：
 * - lua_c_analysis: 验证行为与Lua 5.1.5一致性
 * - lua_with_cpp: 验证现代C++设计模式正确性
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <chrono>

// 测试用的Parser相关头文件（TDD - 先定义接口）
#include "parser/parser.h"
#include "parser/ast.h"
#include "lexer/lexer.h"
#include "lexer/token.h"
#include "core/lua_common.h"
#include "core/lua_errors.h"

using namespace lua_cpp;
using Catch::Approx;

/* ========================================================================== */
/* 测试辅助工具类 */
/* ========================================================================== */

class ParserTestHelper {
public:
    /**
     * @brief 创建Parser实例
     * @param source 源代码字符串
     * @param config Parser配置
     * @return Parser实例
     */
    static std::unique_ptr<Parser> CreateParser(const std::string& source, 
                                               const ParserConfig& config = ParserConfig{}) {
        auto input_stream = std::make_unique<StringInputStream>(source);
        auto lexer = std::make_unique<Lexer>(std::move(input_stream));
        return std::make_unique<Parser>(std::move(lexer), config);
    }

    /**
     * @brief 验证解析成功
     * @param parser Parser实例
     * @param expected_error_count 期望的错误数量
     */
    static void VerifyParseSuccess(const Parser& parser, Size expected_error_count = 0) {
        REQUIRE(parser.GetState() == ParserState::Completed);
        REQUIRE(parser.GetErrorCount() == expected_error_count);
    }

    /**
     * @brief 验证解析错误
     * @param parser Parser实例
     * @param expected_min_errors 最少期望的错误数量
     */
    static void VerifyParseError(const Parser& parser, Size expected_min_errors = 1) {
        REQUIRE(parser.GetState() == ParserState::Error);
        REQUIRE(parser.GetErrorCount() >= expected_min_errors);
    }

    /**
     * @brief 验证AST节点基本属性
     * @param node AST节点
     * @param expected_type 期望的节点类型
     */
    template<typename T>
    static void VerifyASTNode(const std::unique_ptr<T>& node, ASTNodeType expected_type) {
        REQUIRE(node != nullptr);
        REQUIRE(node->GetType() == expected_type);
        REQUIRE(node->GetPosition().IsValid());
    }
};

/* ========================================================================== */
/* 基础语法结构解析测试 */
/* ========================================================================== */

TEST_CASE("Parser - 基础语法结构", "[parser][contract][basic]") {
    SECTION("空程序解析") {
        auto parser = ParserTestHelper::CreateParser("");
        auto program = parser->ParseProgram();
        
        ParserTestHelper::VerifyParseSuccess(*parser);
        ParserTestHelper::VerifyASTNode(program, ASTNodeType::Program);
        REQUIRE(program->GetStatements().size() == 0);
    }

    SECTION("单语句程序") {
        auto parser = ParserTestHelper::CreateParser("return 42");
        auto program = parser->ParseProgram();
        
        ParserTestHelper::VerifyParseSuccess(*parser);
        ParserTestHelper::VerifyASTNode(program, ASTNodeType::Program);
        REQUIRE(program->GetStatements().size() == 1);
        
        auto& stmt = program->GetStatements()[0];
        ParserTestHelper::VerifyASTNode(stmt, ASTNodeType::ReturnStatement);
    }

    SECTION("多语句程序") {
        std::string source = R"(
            local x = 10
            local y = 20
            return x + y
        )";
        
        auto parser = ParserTestHelper::CreateParser(source);
        auto program = parser->ParseProgram();
        
        ParserTestHelper::VerifyParseSuccess(*parser);
        ParserTestHelper::VerifyASTNode(program, ASTNodeType::Program);
        REQUIRE(program->GetStatements().size() == 3);
    }

    SECTION("语句分隔符处理") {
        auto parser = ParserTestHelper::CreateParser("local a = 1; local b = 2; return a + b");
        auto program = parser->ParseProgram();
        
        ParserTestHelper::VerifyParseSuccess(*parser);
        REQUIRE(program->GetStatements().size() == 3);
    }
}

/* ========================================================================== */
/* 表达式解析测试 */
/* ========================================================================== */

TEST_CASE("Parser - 表达式解析", "[parser][contract][expression]") {
    SECTION("字面量表达式") {
        struct TestCase {
            std::string source;
            ASTNodeType expected_type;
            std::string description;
        };
        
        std::vector<TestCase> test_cases = {
            {"42", ASTNodeType::NumberLiteral, "整数字面量"},
            {"3.14", ASTNodeType::NumberLiteral, "浮点数字面量"},
            {"'hello'", ASTNodeType::StringLiteral, "字符串字面量（单引号）"},
            {"\"world\"", ASTNodeType::StringLiteral, "字符串字面量（双引号）"},
            {"true", ASTNodeType::BooleanLiteral, "布尔true字面量"},
            {"false", ASTNodeType::BooleanLiteral, "布尔false字面量"},
            {"nil", ASTNodeType::NilLiteral, "nil字面量"},
            {"...", ASTNodeType::VarargLiteral, "变参字面量"}
        };
        
        for (const auto& test_case : test_cases) {
            INFO("测试用例: " << test_case.description);
            
            auto parser = ParserTestHelper::CreateParser("return " + test_case.source);
            auto program = parser->ParseProgram();
            
            ParserTestHelper::VerifyParseSuccess(*parser);
            
            auto return_stmt = dynamic_cast<ReturnStatement*>(
                program->GetStatements()[0].get());
            REQUIRE(return_stmt != nullptr);
            
            auto& expressions = return_stmt->GetExpressions();
            REQUIRE(expressions.size() == 1);
            
            ParserTestHelper::VerifyASTNode(expressions[0], test_case.expected_type);
        }
    }

    SECTION("二元表达式") {
        struct TestCase {
            std::string source;
            std::string operator_symbol;
            std::string description;
        };
        
        std::vector<TestCase> test_cases = {
            {"1 + 2", "+", "加法运算"},
            {"3 - 4", "-", "减法运算"},
            {"5 * 6", "*", "乘法运算"},
            {"7 / 8", "/", "除法运算"},
            {"9 % 10", "%", "模运算"},
            {"2 ^ 3", "^", "幂运算"},
            {"'a' .. 'b'", "..", "字符串连接"},
            {"1 == 2", "==", "相等比较"},
            {"3 ~= 4", "~=", "不等比较"},
            {"5 < 6", "<", "小于比较"},
            {"7 > 8", ">", "大于比较"},
            {"9 <= 10", "<=", "小于等于比较"},
            {"11 >= 12", ">=", "大于等于比较"},
            {"true and false", "and", "逻辑与"},
            {"true or false", "or", "逻辑或"}
        };
        
        for (const auto& test_case : test_cases) {
            INFO("测试用例: " << test_case.description);
            
            auto parser = ParserTestHelper::CreateParser("return " + test_case.source);
            auto program = parser->ParseProgram();
            
            ParserTestHelper::VerifyParseSuccess(*parser);
            
            auto return_stmt = dynamic_cast<ReturnStatement*>(
                program->GetStatements()[0].get());
            REQUIRE(return_stmt != nullptr);
            
            auto& expressions = return_stmt->GetExpressions();
            REQUIRE(expressions.size() == 1);
            
            auto binary_expr = dynamic_cast<BinaryExpression*>(expressions[0].get());
            REQUIRE(binary_expr != nullptr);
            
            REQUIRE(binary_expr->GetOperator() == test_case.operator_symbol);
            REQUIRE(binary_expr->GetLeft() != nullptr);
            REQUIRE(binary_expr->GetRight() != nullptr);
        }
    }

    SECTION("一元表达式") {
        struct TestCase {
            std::string source;
            std::string operator_symbol;
            std::string description;
        };
        
        std::vector<TestCase> test_cases = {
            {"-42", "-", "一元负号"},
            {"not true", "not", "逻辑非"},
            {"#'hello'", "#", "长度运算符"}
        };
        
        for (const auto& test_case : test_cases) {
            INFO("测试用例: " << test_case.description);
            
            auto parser = ParserTestHelper::CreateParser("return " + test_case.source);
            auto program = parser->ParseProgram();
            
            ParserTestHelper::VerifyParseSuccess(*parser);
            
            auto return_stmt = dynamic_cast<ReturnStatement*>(
                program->GetStatements()[0].get());
            REQUIRE(return_stmt != nullptr);
            
            auto& expressions = return_stmt->GetExpressions();
            REQUIRE(expressions.size() == 1);
            
            auto unary_expr = dynamic_cast<UnaryExpression*>(expressions[0].get());
            REQUIRE(unary_expr != nullptr);
            
            REQUIRE(unary_expr->GetOperator() == test_case.operator_symbol);
            REQUIRE(unary_expr->GetOperand() != nullptr);
        }
    }

    SECTION("运算符优先级") {
        struct TestCase {
            std::string source;
            std::string expected_structure;
            std::string description;
        };
        
        std::vector<TestCase> test_cases = {
            {"1 + 2 * 3", "1 + (2 * 3)", "乘法优先级高于加法"},
            {"2 ^ 3 ^ 4", "2 ^ (3 ^ 4)", "幂运算右结合"},
            {"1 + 2 - 3", "(1 + 2) - 3", "同优先级左结合"},
            {"-2 ^ 3", "-(2 ^ 3)", "一元运算符优先级"},
            {"not a and b", "(not a) and b", "not优先级高于and"},
            {"a or b and c", "a or (b and c)", "and优先级高于or"},
            {"1 < 2 == true", "(1 < 2) == true", "比较优先级高于相等"},
            {"'a' .. 'b' .. 'c'", "'a' .. ('b' .. 'c')", "连接运算符右结合"}
        };
        
        for (const auto& test_case : test_cases) {
            INFO("测试用例: " << test_case.description);
            
            auto parser = ParserTestHelper::CreateParser("return " + test_case.source);
            auto program = parser->ParseProgram();
            
            ParserTestHelper::VerifyParseSuccess(*parser);
            
            // 验证AST结构是否符合期望的优先级
            // 具体验证逻辑需要根据AST设计来实现
            auto return_stmt = dynamic_cast<ReturnStatement*>(
                program->GetStatements()[0].get());
            REQUIRE(return_stmt != nullptr);
            
            auto& expressions = return_stmt->GetExpressions();
            REQUIRE(expressions.size() == 1);
            REQUIRE(expressions[0] != nullptr);
        }
    }
}
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

/* ========================================================================== */
/* 函数相关解析测试 */
/* ========================================================================== */

TEST_CASE("Parser - 函数解析契约", "[parser][contract][function]") {
    SECTION("基础函数定义") {
        std::string source = R"(
            function add(a, b)
                return a + b
            end
        )";
        
        auto parser = ParserTestHelper::CreateParser(source);
        auto program = parser->ParseProgram();
        
        ParserTestHelper::VerifyParseSuccess(*parser);
        REQUIRE(program->GetStatements().size() == 1);
        
        auto func_def = dynamic_cast<FunctionDefinition*>(
            program->GetStatements()[0].get());
        REQUIRE(func_def != nullptr);
        
        REQUIRE(func_def->GetName() == "add");
        REQUIRE(func_def->GetParameters().size() == 2);
        REQUIRE(func_def->GetParameters()[0] == "a");
        REQUIRE(func_def->GetParameters()[1] == "b");
        REQUIRE_FALSE(func_def->IsVariadic());
    }

    SECTION("局部函数定义") {
        std::string source = R"(
            local function multiply(x, y)
                return x * y
            end
        )";
        
        auto parser = ParserTestHelper::CreateParser(source);
        auto program = parser->ParseProgram();
        
        ParserTestHelper::VerifyParseSuccess(*parser);
        
        auto local_func_def = dynamic_cast<LocalFunctionDefinition*>(
            program->GetStatements()[0].get());
        REQUIRE(local_func_def != nullptr);
        
        REQUIRE(local_func_def->GetName() == "multiply");
        REQUIRE(local_func_def->GetParameters().size() == 2);
    }

    SECTION("变参函数") {
        std::string source = R"(
            function varargs(a, b, ...)
                return {...}
            end
        )";
        
        auto parser = ParserTestHelper::CreateParser(source);
        auto program = parser->ParseProgram();
        
        ParserTestHelper::VerifyParseSuccess(*parser);
        
        auto func_def = dynamic_cast<FunctionDefinition*>(
            program->GetStatements()[0].get());
        REQUIRE(func_def != nullptr);
        
        REQUIRE(func_def->GetParameters().size() == 2);
        REQUIRE(func_def->IsVariadic());
    }

    SECTION("函数表达式") {
        std::string source = R"(
            local f = function(x) return x * 2 end
        )";
        
        auto parser = ParserTestHelper::CreateParser(source);
        auto program = parser->ParseProgram();
        
        ParserTestHelper::VerifyParseSuccess(*parser);
        
        auto local_decl = dynamic_cast<LocalDeclaration*>(
            program->GetStatements()[0].get());
        REQUIRE(local_decl != nullptr);
        
        auto& initializers = local_decl->GetInitializers();
        REQUIRE(initializers.size() == 1);
        
        auto func_expr = dynamic_cast<FunctionExpression*>(initializers[0].get());
        REQUIRE(func_expr != nullptr);
        REQUIRE(func_expr->GetParameters().size() == 1);
    }

    SECTION("函数调用") {
        struct TestCase {
            std::string source;
            std::string description;
        };
        
        std::vector<TestCase> test_cases = {
            {"print()", "无参数函数调用"},
            {"print('hello')", "单参数函数调用"},
            {"math.max(1, 2, 3)", "多参数函数调用"},
            {"obj:method()", "方法调用"},
            {"f(g(h()))", "嵌套函数调用"}
        };
        
        for (const auto& test_case : test_cases) {
            INFO("测试用例: " << test_case.description);
            
            auto parser = ParserTestHelper::CreateParser(test_case.source);
            auto program = parser->ParseProgram();
            
            ParserTestHelper::VerifyParseSuccess(*parser);
            REQUIRE(program->GetStatements().size() == 1);
        }
    }
}

/* ========================================================================== */
/* 表操作解析测试 */
/* ========================================================================== */

TEST_CASE("Parser - 表操作契约", "[parser][contract][table]") {
    SECTION("表构造器") {
        struct TestCase {
            std::string source;
            std::string description;
        };
        
        std::vector<TestCase> test_cases = {
            {"{}", "空表"},
            {"{1, 2, 3}", "数组风格表"},
            {"{a = 1, b = 2}", "哈希风格表"},
            {"{1, 2, a = 3, b = 4}", "混合风格表"},
            {"{[1] = 'first', [2] = 'second'}", "显式索引表"},
            {"{'a', 'b', c = 'third'}", "混合索引表"}
        };
        
        for (const auto& test_case : test_cases) {
            INFO("测试用例: " << test_case.description);
            
            auto parser = ParserTestHelper::CreateParser("return " + test_case.source);
            auto program = parser->ParseProgram();
            
            ParserTestHelper::VerifyParseSuccess(*parser);
            
            auto return_stmt = dynamic_cast<ReturnStatement*>(
                program->GetStatements()[0].get());
            REQUIRE(return_stmt != nullptr);
            
            auto& expressions = return_stmt->GetExpressions();
            REQUIRE(expressions.size() == 1);
            
            auto table_constructor = dynamic_cast<TableConstructor*>(expressions[0].get());
            REQUIRE(table_constructor != nullptr);
        }
    }

    SECTION("表访问") {
        struct TestCase {
            std::string source;
            std::string description;
        };
        
        std::vector<TestCase> test_cases = {
            {"t[1]", "数字索引访问"},
            {"t['key']", "字符串索引访问"},
            {"t.field", "字段访问"},
            {"t[expr]", "表达式索引访问"},
            {"t[1][2]", "嵌套索引访问"},
            {"t.a.b.c", "链式字段访问"},
            {"t[1].field", "混合访问方式"}
        };
        
        for (const auto& test_case : test_cases) {
            INFO("测试用例: " << test_case.description);
            
            auto parser = ParserTestHelper::CreateParser("return " + test_case.source);
            auto program = parser->ParseProgram();
            
            ParserTestHelper::VerifyParseSuccess(*parser);
        }
    }
}

/* ========================================================================== */
/* 控制流语句解析测试 */
/* ========================================================================== */

TEST_CASE("Parser - 控制流契约", "[parser][contract][control_flow]") {
    SECTION("if语句") {
        std::string source = R"(
            if x > 0 then
                print("positive")
            elseif x < 0 then
                print("negative")
            else
                print("zero")
            end
        )";
        
        auto parser = ParserTestHelper::CreateParser(source);
        auto program = parser->ParseProgram();
        
        ParserTestHelper::VerifyParseSuccess(*parser);
        
        auto if_stmt = dynamic_cast<IfStatement*>(
            program->GetStatements()[0].get());
        REQUIRE(if_stmt != nullptr);
        
        REQUIRE(if_stmt->GetCondition() != nullptr);
        REQUIRE(if_stmt->GetThenBlock() != nullptr);
        REQUIRE(if_stmt->GetElseIfClauses().size() == 1);
        REQUIRE(if_stmt->GetElseBlock() != nullptr);
    }

    SECTION("while循环") {
        std::string source = R"(
            while i < 10 do
                i = i + 1
            end
        )";
        
        auto parser = ParserTestHelper::CreateParser(source);
        auto program = parser->ParseProgram();
        
        ParserTestHelper::VerifyParseSuccess(*parser);
        
        auto while_stmt = dynamic_cast<WhileStatement*>(
            program->GetStatements()[0].get());
        REQUIRE(while_stmt != nullptr);
        
        REQUIRE(while_stmt->GetCondition() != nullptr);
        REQUIRE(while_stmt->GetBody() != nullptr);
    }

    SECTION("repeat循环") {
        std::string source = R"(
            repeat
                i = i + 1
            until i >= 10
        )";
        
        auto parser = ParserTestHelper::CreateParser(source);
        auto program = parser->ParseProgram();
        
        ParserTestHelper::VerifyParseSuccess(*parser);
        
        auto repeat_stmt = dynamic_cast<RepeatStatement*>(
            program->GetStatements()[0].get());
        REQUIRE(repeat_stmt != nullptr);
        
        REQUIRE(repeat_stmt->GetBody() != nullptr);
        REQUIRE(repeat_stmt->GetCondition() != nullptr);
    }

    SECTION("数值for循环") {
        std::string source = R"(
            for i = 1, 10, 2 do
                print(i)
            end
        )";
        
        auto parser = ParserTestHelper::CreateParser(source);
        auto program = parser->ParseProgram();
        
        ParserTestHelper::VerifyParseSuccess(*parser);
        
        auto for_stmt = dynamic_cast<NumericForStatement*>(
            program->GetStatements()[0].get());
        REQUIRE(for_stmt != nullptr);
        
        REQUIRE(for_stmt->GetVariable() == "i");
        REQUIRE(for_stmt->GetInitial() != nullptr);
        REQUIRE(for_stmt->GetLimit() != nullptr);
        REQUIRE(for_stmt->GetStep() != nullptr);
        REQUIRE(for_stmt->GetBody() != nullptr);
    }

    SECTION("泛型for循环") {
        std::string source = R"(
            for k, v in pairs(t) do
                print(k, v)
            end
        )";
        
        auto parser = ParserTestHelper::CreateParser(source);
        auto program = parser->ParseProgram();
        
        ParserTestHelper::VerifyParseSuccess(*parser);
        
        auto for_stmt = dynamic_cast<GenericForStatement*>(
            program->GetStatements()[0].get());
        REQUIRE(for_stmt != nullptr);
        
        REQUIRE(for_stmt->GetVariables().size() == 2);
        REQUIRE(for_stmt->GetExpressions().size() == 1);
        REQUIRE(for_stmt->GetBody() != nullptr);
    }
}

/* ========================================================================== */
/* 错误处理测试 */
/* ========================================================================== */

TEST_CASE("Parser - 错误处理契约", "[parser][contract][error]") {
    SECTION("语法错误检测") {
        struct TestCase {
            std::string source;
            std::string description;
        };
        
        std::vector<TestCase> error_cases = {
            {"if true then", "未闭合的if语句"},
            {"function f()", "未闭合的函数定义"},
            {"local a =", "不完整的赋值"},
            {"{1, 2,", "未闭合的表构造"},
            {"return)", "不匹配的括号"},
            {"for i = 1", "不完整的for循环"},
            {"repeat i = i + 1", "缺少until的repeat"},
            {"elseif true then", "孤立的elseif"},
            {"end", "孤立的end"},
            {"function 123()", "无效的函数名"}
        };
        
        for (const auto& test_case : error_cases) {
            INFO("测试用例: " << test_case.description);
            
            auto parser = ParserTestHelper::CreateParser(test_case.source);
            
            REQUIRE_THROWS_AS(parser->ParseProgram(), SyntaxError);
            ParserTestHelper::VerifyParseError(*parser);
        }
    }

    SECTION("错误恢复机制") {
        std::string source = R"(
            local a = 1
            function invalid syntax here
            local b = 2
            return b
        )";
        
        ParserConfig config;
        config.recover_from_errors = true;
        
        auto parser = ParserTestHelper::CreateParser(source, config);
        
        std::unique_ptr<Program> program;
        REQUIRE_NOTHROW(program = parser->ParseProgram());
        
        // 应该有错误，但是能够恢复并继续解析
        REQUIRE(parser->GetErrorCount() > 0);
        
        // 检查是否正确恢复和解析了后续的语句
        if (program) {
            // 具体的恢复验证逻辑
            REQUIRE(program->GetStatements().size() > 0);
        }
    }
}

/* ========================================================================== */
/* 边界条件和性能测试 */
/* ========================================================================== */

TEST_CASE("Parser - 边界条件契约", "[parser][contract][boundary]") {
    SECTION("深度嵌套结构") {
        std::stringstream source;
        const int nesting_depth = 50;
        
        // 构造深度嵌套的if语句
        for (int i = 0; i < nesting_depth; ++i) {
            source << "if true then ";
        }
        source << "return 1 ";
        for (int i = 0; i < nesting_depth; ++i) {
            source << "end ";
        }
        
        auto parser = ParserTestHelper::CreateParser(source.str());
        auto program = parser->ParseProgram();
        
        ParserTestHelper::VerifyParseSuccess(*parser);
        REQUIRE(program->GetStatements().size() == 1);
    }

    SECTION("长表达式链") {
        std::stringstream source;
        source << "return ";
        
        const int expression_length = 100;
        for (int i = 0; i < expression_length; ++i) {
            if (i > 0) source << " + ";
            source << i;
        }
        
        auto parser = ParserTestHelper::CreateParser(source.str());
        auto program = parser->ParseProgram();
        
        ParserTestHelper::VerifyParseSuccess(*parser);
        
        auto return_stmt = dynamic_cast<ReturnStatement*>(
            program->GetStatements()[0].get());
        REQUIRE(return_stmt != nullptr);
        
        auto& expressions = return_stmt->GetExpressions();
        REQUIRE(expressions.size() == 1);
        REQUIRE(expressions[0] != nullptr);
    }

    SECTION("递归深度限制") {
        ParserConfig config;
        config.max_recursion_depth = 10;
        
        // 构造超过递归深度限制的输入
        std::stringstream source;
        for (int i = 0; i < 20; ++i) {
            source << "(";
        }
        source << "1";
        for (int i = 0; i < 20; ++i) {
            source << ")";
        }
        
        auto parser = ParserTestHelper::CreateParser("return " + source.str(), config);
        
        REQUIRE_THROWS_AS(parser->ParseProgram(), LuaError);
    }
}

/* ========================================================================== */
/* AST验证测试 */
/* ========================================================================== */

TEST_CASE("Parser - AST验证契约", "[parser][contract][ast]") {
    SECTION("节点位置信息") {
        std::string source = R"(
            local x = 42
            return x
        )";
        
        auto parser = ParserTestHelper::CreateParser(source);
        auto program = parser->ParseProgram();
        
        ParserTestHelper::VerifyParseSuccess(*parser);
        
        // 验证所有AST节点都有正确的位置信息
        for (const auto& stmt : program->GetStatements()) {
            REQUIRE(stmt->GetPosition().IsValid());
            REQUIRE(stmt->GetPosition().line > 0);
            REQUIRE(stmt->GetPosition().column > 0);
        }
    }

    SECTION("节点父子关系") {
        std::string source = R"(
            if x > 0 then
                return x
            end
        )";
        
        auto parser = ParserTestHelper::CreateParser(source);
        auto program = parser->ParseProgram();
        
        ParserTestHelper::VerifyParseSuccess(*parser);
        
        // 验证父子节点关系的正确性
        auto if_stmt = dynamic_cast<IfStatement*>(
            program->GetStatements()[0].get());
        REQUIRE(if_stmt != nullptr);
        
        // 验证条件表达式
        auto condition = if_stmt->GetCondition();
        REQUIRE(condition != nullptr);
        
        // 验证then块
        auto then_block = if_stmt->GetThenBlock();
        REQUIRE(then_block != nullptr);
    }
}

/* ========================================================================== */
/* 性能测试 */
/* ========================================================================== */

TEST_CASE("Parser - 性能契约", "[parser][contract][performance]") {
    SECTION("大型程序解析性能") {
        // 生成大型程序用于性能测试
        std::stringstream source;
        const int num_functions = 1000;
        
        for (int i = 0; i < num_functions; ++i) {
            source << "function func" << i << "(a, b)\n";
            source << "  return a + b + " << i << "\n";
            source << "end\n\n";
        }
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        auto parser = ParserTestHelper::CreateParser(source.str());
        auto program = parser->ParseProgram();
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time);
        
        ParserTestHelper::VerifyParseSuccess(*parser);
        REQUIRE(program->GetStatements().size() == num_functions);
        
        // 性能要求：解析1000个函数应该在合理时间内完成
        INFO("解析时间: " << duration.count() << "ms");
        REQUIRE(duration.count() < 1000);
    }
}

/* ========================================================================== */
/* Lua 5.1.5兼容性测试 */
/* ========================================================================== */

TEST_CASE("Parser - Lua 5.1.5兼容性契约", "[parser][contract][compatibility]") {
    SECTION("Lua 5.1特有语法特性") {
        struct TestCase {
            std::string source;
            std::string description;
        };
        
        std::vector<TestCase> lua51_features = {
            {"local function f() end", "局部函数定义"},
            {"for i = 1, 10 do end", "数值for循环"},
            {"for k, v in pairs(t) do end", "泛型for循环"},
            {"function f(...) return ... end", "变参函数"},
            {"local a, b = 1, 2", "多重赋值"},
            {"return function() end", "函数表达式返回"},
            {"t = {a = 1, [2] = 'two'}", "表构造器混合语法"},
            {"obj:method(args)", "方法调用语法"}
        };
        
        for (const auto& test_case : lua51_features) {
            INFO("Lua 5.1特性: " << test_case.description);
            
            auto parser = ParserTestHelper::CreateParser(test_case.source);
            auto program = parser->ParseProgram();
            
            ParserTestHelper::VerifyParseSuccess(*parser);
        }
    }
}