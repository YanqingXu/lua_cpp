/**
 * @file test_lexer_contract.cpp
 * @brief Lexer（词法分析器）契约测试
 * @description 测试Lua词法分析器的所有行为契约，确保100% Lua 5.1.5兼容性
 *              包括Token类型、词法规则、错误处理、前瞻机制等核心功能
 * @date 2025-09-20
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/generators/catch_generators.hpp>

// 注意：这些头文件还不存在，这是TDD方法 - 先写测试定义接口
#include "lexer/token.h"
#include "lexer/lexer.h"
#include "core/lua_common.h"
#include "core/lua_errors.h"

using namespace lua_cpp;
using Catch::Approx;

/* ========================================================================== */
/* Token类型和基础结构契约 */
/* ========================================================================== */

TEST_CASE("Token - 类型定义契约", "[lexer][contract][token]") {
    SECTION("TokenType枚举应该包含所有Lua 5.1.5标准Token") {
        // 保留字Token
        static_assert(static_cast<int>(TokenType::And) >= 257);
        static_assert(static_cast<int>(TokenType::Break) > static_cast<int>(TokenType::And));
        static_assert(static_cast<int>(TokenType::Do) > static_cast<int>(TokenType::Break));
        static_assert(static_cast<int>(TokenType::Else) > static_cast<int>(TokenType::Do));
        static_assert(static_cast<int>(TokenType::ElseIf) > static_cast<int>(TokenType::Else));
        static_assert(static_cast<int>(TokenType::End) > static_cast<int>(TokenType::ElseIf));
        static_assert(static_cast<int>(TokenType::False) > static_cast<int>(TokenType::End));
        static_assert(static_cast<int>(TokenType::For) > static_cast<int>(TokenType::False));
        static_assert(static_cast<int>(TokenType::Function) > static_cast<int>(TokenType::For));
        static_assert(static_cast<int>(TokenType::If) > static_cast<int>(TokenType::Function));
        static_assert(static_cast<int>(TokenType::In) > static_cast<int>(TokenType::If));
        static_assert(static_cast<int>(TokenType::Local) > static_cast<int>(TokenType::In));
        static_assert(static_cast<int>(TokenType::Nil) > static_cast<int>(TokenType::Local));
        static_assert(static_cast<int>(TokenType::Not) > static_cast<int>(TokenType::Nil));
        static_assert(static_cast<int>(TokenType::Or) > static_cast<int>(TokenType::Not));
        static_assert(static_cast<int>(TokenType::Repeat) > static_cast<int>(TokenType::Or));
        static_assert(static_cast<int>(TokenType::Return) > static_cast<int>(TokenType::Repeat));
        static_assert(static_cast<int>(TokenType::Then) > static_cast<int>(TokenType::Return));
        static_assert(static_cast<int>(TokenType::True) > static_cast<int>(TokenType::Then));
        static_assert(static_cast<int>(TokenType::Until) > static_cast<int>(TokenType::True));
        static_assert(static_cast<int>(TokenType::While) > static_cast<int>(TokenType::Until));
        
        // 多字符操作符Token
        static_assert(static_cast<int>(TokenType::Concat) > static_cast<int>(TokenType::While));   // ..
        static_assert(static_cast<int>(TokenType::Dots) > static_cast<int>(TokenType::Concat));    // ...
        static_assert(static_cast<int>(TokenType::Equal) > static_cast<int>(TokenType::Dots));     // ==
        static_assert(static_cast<int>(TokenType::GreaterEqual) > static_cast<int>(TokenType::Equal)); // >=
        static_assert(static_cast<int>(TokenType::LessEqual) > static_cast<int>(TokenType::GreaterEqual)); // <=
        static_assert(static_cast<int>(TokenType::NotEqual) > static_cast<int>(TokenType::LessEqual)); // ~=
        
        // 字面量Token
        static_assert(static_cast<int>(TokenType::Number) > static_cast<int>(TokenType::NotEqual));
        static_assert(static_cast<int>(TokenType::String) > static_cast<int>(TokenType::Number));
        static_assert(static_cast<int>(TokenType::Name) > static_cast<int>(TokenType::String));
        
        // 特殊Token
        static_assert(static_cast<int>(TokenType::EndOfSource) > static_cast<int>(TokenType::Name));
    }

    SECTION("单字符Token应该使用ASCII值") {
        // 算术操作符
        REQUIRE(static_cast<int>(TokenType::Plus) == '+');
        REQUIRE(static_cast<int>(TokenType::Minus) == '-');
        REQUIRE(static_cast<int>(TokenType::Multiply) == '*');
        REQUIRE(static_cast<int>(TokenType::Divide) == '/');
        REQUIRE(static_cast<int>(TokenType::Modulo) == '%');
        REQUIRE(static_cast<int>(TokenType::Power) == '^');
        
        // 关系操作符
        REQUIRE(static_cast<int>(TokenType::Less) == '<');
        REQUIRE(static_cast<int>(TokenType::Greater) == '>');
        
        // 逻辑操作符
        REQUIRE(static_cast<int>(TokenType::Length) == '#');
        
        // 分隔符
        REQUIRE(static_cast<int>(TokenType::LeftParen) == '(');
        REQUIRE(static_cast<int>(TokenType::RightParen) == ')');
        REQUIRE(static_cast<int>(TokenType::LeftBrace) == '{');
        REQUIRE(static_cast<int>(TokenType::RightBrace) == '}');
        REQUIRE(static_cast<int>(TokenType::LeftBracket) == '[');
        REQUIRE(static_cast<int>(TokenType::RightBracket) == ']');
        REQUIRE(static_cast<int>(TokenType::Semicolon) == ';');
        REQUIRE(static_cast<int>(TokenType::Comma) == ',');
        REQUIRE(static_cast<int>(TokenType::Dot) == '.');
        REQUIRE(static_cast<int>(TokenType::Assign) == '=');
    }
}

TEST_CASE("Token - 基础结构契约", "[lexer][contract][token]") {
    SECTION("Token应该包含类型和位置信息") {
        Token token;
        REQUIRE(token.GetType() == TokenType::EndOfSource);  // 默认值
        REQUIRE(token.GetLine() == 1);                       // 默认行号
        REQUIRE(token.GetColumn() == 1);                     // 默认列号
    }

    SECTION("Token应该支持不同类型的语义值") {
        // 数字Token
        Token numberToken = Token::CreateNumber(42.5, 1, 1);
        REQUIRE(numberToken.GetType() == TokenType::Number);
        REQUIRE(numberToken.GetNumber() == Approx(42.5));
        
        // 字符串Token
        Token stringToken = Token::CreateString("hello", 1, 5);
        REQUIRE(stringToken.GetType() == TokenType::String);
        REQUIRE(stringToken.GetString() == "hello");
        
        // 标识符Token
        Token nameToken = Token::CreateName("variable", 2, 1);
        REQUIRE(nameToken.GetType() == TokenType::Name);
        REQUIRE(nameToken.GetString() == "variable");
        
        // 关键字Token
        Token keywordToken = Token::CreateKeyword(TokenType::Function, 3, 1);
        REQUIRE(keywordToken.GetType() == TokenType::Function);
        
        // 操作符Token
        Token operatorToken = Token::CreateOperator(TokenType::Plus, 4, 5);
        REQUIRE(operatorToken.GetType() == TokenType::Plus);
    }

    SECTION("Token应该支持复制和移动语义") {
        Token original = Token::CreateString("test", 1, 1);
        
        // 复制构造
        Token copied = original;
        REQUIRE(copied.GetType() == TokenType::String);
        REQUIRE(copied.GetString() == "test");
        
        // 移动构造
        Token moved = std::move(original);
        REQUIRE(moved.GetType() == TokenType::String);
        REQUIRE(moved.GetString() == "test");
        
        // 复制赋值
        Token assigned;
        assigned = copied;
        REQUIRE(assigned.GetType() == TokenType::String);
        REQUIRE(assigned.GetString() == "test");
    }
}

/* ========================================================================== */
/* Lexer基础功能契约 */
/* ========================================================================== */

TEST_CASE("Lexer - 构造和初始化契约", "[lexer][contract][basic]") {
    SECTION("Lexer应该正确初始化") {
        std::string source = "print('hello')";
        Lexer lexer(source, "test.lua");
        
        REQUIRE(lexer.GetSourceName() == "test.lua");
        REQUIRE(lexer.GetCurrentLine() == 1);
        REQUIRE(lexer.GetCurrentColumn() == 1);
        REQUIRE_FALSE(lexer.IsAtEnd());
    }

    SECTION("空源码应该立即到达结束") {
        std::string source = "";
        Lexer lexer(source, "empty.lua");
        
        Token token = lexer.NextToken();
        REQUIRE(token.GetType() == TokenType::EndOfSource);
        REQUIRE(lexer.IsAtEnd());
    }

    SECTION("只有空白符的源码应该到达结束") {
        std::string source = "   \t\n\r  ";
        Lexer lexer(source, "whitespace.lua");
        
        Token token = lexer.NextToken();
        REQUIRE(token.GetType() == TokenType::EndOfSource);
        REQUIRE(lexer.IsAtEnd());
    }
}

TEST_CASE("Lexer - 前瞻机制契约", "[lexer][contract][lookahead]") {
    SECTION("PeekToken应该不改变词法器状态") {
        std::string source = "local x = 42";
        Lexer lexer(source, "test.lua");
        
        Size originalLine = lexer.GetCurrentLine();
        Size originalColumn = lexer.GetCurrentColumn();
        
        Token peeked = lexer.PeekToken();
        REQUIRE(peeked.GetType() == TokenType::Local);
        
        // 状态不应该改变
        REQUIRE(lexer.GetCurrentLine() == originalLine);
        REQUIRE(lexer.GetCurrentColumn() == originalColumn);
        
        // 后续NextToken应该返回相同的Token
        Token next = lexer.NextToken();
        REQUIRE(next.GetType() == peeked.GetType());
    }

    SECTION("连续Peek应该返回相同结果") {
        std::string source = "function test() end";
        Lexer lexer(source, "test.lua");
        
        Token peek1 = lexer.PeekToken();
        Token peek2 = lexer.PeekToken();
        
        REQUIRE(peek1.GetType() == peek2.GetType());
        REQUIRE(peek1.GetLine() == peek2.GetLine());
        REQUIRE(peek1.GetColumn() == peek2.GetColumn());
    }
}

/* ========================================================================== */
/* 标识符和关键字识别契约 */
/* ========================================================================== */

TEST_CASE("Lexer - 标识符识别契约", "[lexer][contract][identifier]") {
    SECTION("合法标识符应该被正确识别") {
        auto testCases = GENERATE(
            std::make_pair("a", "a"),
            std::make_pair("_", "_"),
            std::make_pair("variable", "variable"),
            std::make_pair("_var", "_var"),
            std::make_pair("var123", "var123"),
            std::make_pair("_123", "_123"),
            std::make_pair("CamelCase", "CamelCase"),
            std::make_pair("snake_case", "snake_case"),
            std::make_pair("UPPER_CASE", "UPPER_CASE"),
            std::make_pair("mixedCASE_123", "mixedCASE_123")
        );

        std::string source = testCases.first;
        Lexer lexer(source, "test.lua");
        
        Token token = lexer.NextToken();
        REQUIRE(token.GetType() == TokenType::Name);
        REQUIRE(token.GetString() == testCases.second);
    }

    SECTION("标识符不能以数字开头") {
        std::string source = "123abc";
        Lexer lexer(source, "test.lua");
        
        // 应该识别为数字123，然后是标识符abc
        Token token1 = lexer.NextToken();
        REQUIRE(token1.GetType() == TokenType::Number);
        REQUIRE(token1.GetNumber() == Approx(123));
        
        Token token2 = lexer.NextToken();
        REQUIRE(token2.GetType() == TokenType::Name);
        REQUIRE(token2.GetString() == "abc");
    }

    SECTION("标识符可以包含Unicode字符") {
        // Lua 5.1支持UTF-8标识符
        std::string source = "变量";
        Lexer lexer(source, "test.lua");
        
        Token token = lexer.NextToken();
        REQUIRE(token.GetType() == TokenType::Name);
        REQUIRE(token.GetString() == "变量");
    }
}

TEST_CASE("Lexer - 关键字识别契约", "[lexer][contract][keyword]") {
    SECTION("所有Lua 5.1.5关键字应该被正确识别") {
        auto keywords = GENERATE(
            std::make_pair("and", TokenType::And),
            std::make_pair("break", TokenType::Break),
            std::make_pair("do", TokenType::Do),
            std::make_pair("else", TokenType::Else),
            std::make_pair("elseif", TokenType::ElseIf),
            std::make_pair("end", TokenType::End),
            std::make_pair("false", TokenType::False),
            std::make_pair("for", TokenType::For),
            std::make_pair("function", TokenType::Function),
            std::make_pair("if", TokenType::If),
            std::make_pair("in", TokenType::In),
            std::make_pair("local", TokenType::Local),
            std::make_pair("nil", TokenType::Nil),
            std::make_pair("not", TokenType::Not),
            std::make_pair("or", TokenType::Or),
            std::make_pair("repeat", TokenType::Repeat),
            std::make_pair("return", TokenType::Return),
            std::make_pair("then", TokenType::Then),
            std::make_pair("true", TokenType::True),
            std::make_pair("until", TokenType::Until),
            std::make_pair("while", TokenType::While)
        );

        std::string source = keywords.first;
        Lexer lexer(source, "test.lua");
        
        Token token = lexer.NextToken();
        REQUIRE(token.GetType() == keywords.second);
    }

    SECTION("关键字区分大小写") {
        auto testCases = GENERATE(
            "And", "AND", "Break", "BREAK", "Do", "DO",
            "Else", "ELSE", "End", "END", "Function", "FUNCTION"
        );

        std::string source = testCases;
        Lexer lexer(source, "test.lua");
        
        Token token = lexer.NextToken();
        REQUIRE(token.GetType() == TokenType::Name);  // 应该识别为标识符，不是关键字
        REQUIRE(token.GetString() == testCases);
    }
}

/* ========================================================================== */
/* 数字字面量识别契约 */
/* ========================================================================== */

TEST_CASE("Lexer - 整数识别契约", "[lexer][contract][number][integer]") {
    SECTION("十进制整数应该被正确识别") {
        auto testCases = GENERATE(
            std::make_pair("0", 0.0),
            std::make_pair("1", 1.0),
            std::make_pair("123", 123.0),
            std::make_pair("999999", 999999.0)
        );

        std::string source = testCases.first;
        Lexer lexer(source, "test.lua");
        
        Token token = lexer.NextToken();
        REQUIRE(token.GetType() == TokenType::Number);
        REQUIRE(token.GetNumber() == Approx(testCases.second));
    }

    SECTION("十六进制整数应该被正确识别") {
        auto testCases = GENERATE(
            std::make_pair("0x0", 0.0),
            std::make_pair("0x1", 1.0),
            std::make_pair("0xa", 10.0),
            std::make_pair("0xA", 10.0),
            std::make_pair("0xff", 255.0),
            std::make_pair("0xFF", 255.0),
            std::make_pair("0x123", 291.0),
            std::make_pair("0xABCDEF", 11259375.0)
        );

        std::string source = testCases.first;
        Lexer lexer(source, "test.lua");
        
        Token token = lexer.NextToken();
        REQUIRE(token.GetType() == TokenType::Number);
        REQUIRE(token.GetNumber() == Approx(testCases.second));
    }
}

TEST_CASE("Lexer - 浮点数识别契约", "[lexer][contract][number][float]") {
    SECTION("简单浮点数应该被正确识别") {
        auto testCases = GENERATE(
            std::make_pair("0.0", 0.0),
            std::make_pair("1.0", 1.0),
            std::make_pair("3.14", 3.14),
            std::make_pair("123.456", 123.456),
            std::make_pair(".5", 0.5),
            std::make_pair("5.", 5.0)
        );

        std::string source = testCases.first;
        Lexer lexer(source, "test.lua");
        
        Token token = lexer.NextToken();
        REQUIRE(token.GetType() == TokenType::Number);
        REQUIRE(token.GetNumber() == Approx(testCases.second));
    }

    SECTION("科学计数法应该被正确识别") {
        auto testCases = GENERATE(
            std::make_pair("1e0", 1.0),
            std::make_pair("1E0", 1.0),
            std::make_pair("1e1", 10.0),
            std::make_pair("1e-1", 0.1),
            std::make_pair("1.5e2", 150.0),
            std::make_pair("1.5E-2", 0.015),
            std::make_pair("123.456e3", 123456.0),
            std::make_pair("0.5e-1", 0.05)
        );

        std::string source = testCases.first;
        Lexer lexer(source, "test.lua");
        
        Token token = lexer.NextToken();
        REQUIRE(token.GetType() == TokenType::Number);
        REQUIRE(token.GetNumber() == Approx(testCases.second));
    }

    SECTION("十六进制浮点数应该被正确识别") {
        auto testCases = GENERATE(
            std::make_pair("0x1.0", 1.0),
            std::make_pair("0xa.b", 10.6875),  // 10 + 11/16
            std::make_pair("0x1p0", 1.0),      // 1 * 2^0
            std::make_pair("0x1p1", 2.0),      // 1 * 2^1
            std::make_pair("0x1p-1", 0.5),     // 1 * 2^-1
            std::make_pair("0x1.8p0", 1.5)     // 1.5 * 2^0
        );

        std::string source = testCases.first;
        Lexer lexer(source, "test.lua");
        
        Token token = lexer.NextToken();
        REQUIRE(token.GetType() == TokenType::Number);
        REQUIRE(token.GetNumber() == Approx(testCases.second));
    }
}

/* ========================================================================== */
/* 字符串字面量识别契约 */
/* ========================================================================== */

TEST_CASE("Lexer - 简单字符串识别契约", "[lexer][contract][string][simple]") {
    SECTION("双引号字符串应该被正确识别") {
        auto testCases = GENERATE(
            std::make_pair("\"\"", ""),
            std::make_pair("\"hello\"", "hello"),
            std::make_pair("\"Hello, World!\"", "Hello, World!"),
            std::make_pair("\"123\"", "123")
        );

        std::string source = testCases.first;
        Lexer lexer(source, "test.lua");
        
        Token token = lexer.NextToken();
        REQUIRE(token.GetType() == TokenType::String);
        REQUIRE(token.GetString() == testCases.second);
    }

    SECTION("单引号字符串应该被正确识别") {
        auto testCases = GENERATE(
            std::make_pair("''", ""),
            std::make_pair("'hello'", "hello"),
            std::make_pair("'Hello, World!'", "Hello, World!"),
            std::make_pair("'123'", "123")
        );

        std::string source = testCases.first;
        Lexer lexer(source, "test.lua");
        
        Token token = lexer.NextToken();
        REQUIRE(token.GetType() == TokenType::String);
        REQUIRE(token.GetString() == testCases.second);
    }
}

TEST_CASE("Lexer - 转义序列识别契约", "[lexer][contract][string][escape]") {
    SECTION("标准转义序列应该被正确处理") {
        auto testCases = GENERATE(
            std::make_pair("\"\\n\"", "\n"),      // 换行
            std::make_pair("\"\\r\"", "\r"),      // 回车
            std::make_pair("\"\\t\"", "\t"),      // 制表符
            std::make_pair("\"\\b\"", "\b"),      // 退格
            std::make_pair("\"\\f\"", "\f"),      // 换页
            std::make_pair("\"\\v\"", "\v"),      // 垂直制表符
            std::make_pair("\"\\a\"", "\a"),      // 响铃
            std::make_pair("\"\\\\\"", "\\"),     // 反斜杠
            std::make_pair("\"\\\"\"", "\""),     // 双引号
            std::make_pair("\"\\'\"", "'")        // 单引号
        );

        std::string source = testCases.first;
        Lexer lexer(source, "test.lua");
        
        Token token = lexer.NextToken();
        REQUIRE(token.GetType() == TokenType::String);
        REQUIRE(token.GetString() == testCases.second);
    }

    SECTION("数字转义序列应该被正确处理") {
        auto testCases = GENERATE(
            std::make_pair("\"\\0\"", std::string(1, '\0')),     // \\0
            std::make_pair("\"\\65\"", "A"),                      // \\65 = ASCII 65 = 'A'
            std::make_pair("\"\\097\"", "a"),                     // \\097 = ASCII 97 = 'a'
            std::make_pair("\"\\255\"", std::string(1, '\xFF'))   // \\255
        );

        std::string source = testCases.first;
        Lexer lexer(source, "test.lua");
        
        Token token = lexer.NextToken();
        REQUIRE(token.GetType() == TokenType::String);
        REQUIRE(token.GetString() == testCases.second);
    }
}

TEST_CASE("Lexer - 长字符串识别契约", "[lexer][contract][string][long]") {
    SECTION("基本长字符串应该被正确识别") {
        auto testCases = GENERATE(
            std::make_pair("[[]]", ""),
            std::make_pair("[[hello]]", "hello"),
            std::make_pair("[[Hello\nWorld]]", "Hello\nWorld"),
            std::make_pair("[[no escape \\n sequences]]", "no escape \\n sequences")
        );

        std::string source = testCases.first;
        Lexer lexer(source, "test.lua");
        
        Token token = lexer.NextToken();
        REQUIRE(token.GetType() == TokenType::String);
        REQUIRE(token.GetString() == testCases.second);
    }

    SECTION("嵌套等号的长字符串应该被正确识别") {
        auto testCases = GENERATE(
            std::make_pair("[=[]=]", ""),
            std::make_pair("[=[hello]=]", "hello"),
            std::make_pair("[===[hello [[ world ]] !]===]", "hello [[ world ]] !"),
            std::make_pair("[====[can contain [=[ and ]=] sequences]====]", "can contain [=[ and ]=] sequences")
        );

        std::string source = testCases.first;
        Lexer lexer(source, "test.lua");
        
        Token token = lexer.NextToken();
        REQUIRE(token.GetType() == TokenType::String);
        REQUIRE(token.GetString() == testCases.second);
    }

    SECTION("长字符串应该保留换行符") {
        std::string source = "[[\nfirst line\nsecond line\n]]";
        Lexer lexer(source, "test.lua");
        
        Token token = lexer.NextToken();
        REQUIRE(token.GetType() == TokenType::String);
        REQUIRE(token.GetString() == "\nfirst line\nsecond line\n");
    }
}

/* ========================================================================== */
/* 操作符识别契约 */
/* ========================================================================== */

TEST_CASE("Lexer - 单字符操作符识别契约", "[lexer][contract][operator][single]") {
    SECTION("算术操作符应该被正确识别") {
        auto operators = GENERATE(
            std::make_pair("+", TokenType::Plus),
            std::make_pair("-", TokenType::Minus),
            std::make_pair("*", TokenType::Multiply),
            std::make_pair("/", TokenType::Divide),
            std::make_pair("%", TokenType::Modulo),
            std::make_pair("^", TokenType::Power)
        );

        std::string source = operators.first;
        Lexer lexer(source, "test.lua");
        
        Token token = lexer.NextToken();
        REQUIRE(token.GetType() == operators.second);
    }

    SECTION("关系操作符应该被正确识别") {
        auto operators = GENERATE(
            std::make_pair("<", TokenType::Less),
            std::make_pair(">", TokenType::Greater),
            std::make_pair("=", TokenType::Assign)
        );

        std::string source = operators.first;
        Lexer lexer(source, "test.lua");
        
        Token token = lexer.NextToken();
        REQUIRE(token.GetType() == operators.second);
    }

    SECTION("其他操作符应该被正确识别") {
        auto operators = GENERATE(
            std::make_pair("#", TokenType::Length),
            std::make_pair(".", TokenType::Dot)
        );

        std::string source = operators.first;
        Lexer lexer(source, "test.lua");
        
        Token token = lexer.NextToken();
        REQUIRE(token.GetType() == operators.second);
    }
}

TEST_CASE("Lexer - 多字符操作符识别契约", "[lexer][contract][operator][multi]") {
    SECTION("双字符操作符应该被正确识别") {
        auto operators = GENERATE(
            std::make_pair("..", TokenType::Concat),
            std::make_pair("==", TokenType::Equal),
            std::make_pair("~=", TokenType::NotEqual),
            std::make_pair("<=", TokenType::LessEqual),
            std::make_pair(">=", TokenType::GreaterEqual)
        );

        std::string source = operators.first;
        Lexer lexer(source, "test.lua");
        
        Token token = lexer.NextToken();
        REQUIRE(token.GetType() == operators.second);
    }

    SECTION("三字符操作符应该被正确识别") {
        std::string source = "...";
        Lexer lexer(source, "test.lua");
        
        Token token = lexer.NextToken();
        REQUIRE(token.GetType() == TokenType::Dots);
    }

    SECTION("操作符前缀应该被正确区分") {
        // . vs .. vs ...
        std::string source = ". .. ...";
        Lexer lexer(source, "test.lua");
        
        Token token1 = lexer.NextToken();
        REQUIRE(token1.GetType() == TokenType::Dot);
        
        Token token2 = lexer.NextToken();
        REQUIRE(token2.GetType() == TokenType::Concat);
        
        Token token3 = lexer.NextToken();
        REQUIRE(token3.GetType() == TokenType::Dots);
    }
}

/* ========================================================================== */
/* 分隔符识别契约 */
/* ========================================================================== */

TEST_CASE("Lexer - 分隔符识别契约", "[lexer][contract][delimiter]") {
    SECTION("括号类分隔符应该被正确识别") {
        auto delimiters = GENERATE(
            std::make_pair("(", TokenType::LeftParen),
            std::make_pair(")", TokenType::RightParen),
            std::make_pair("{", TokenType::LeftBrace),
            std::make_pair("}", TokenType::RightBrace),
            std::make_pair("[", TokenType::LeftBracket),
            std::make_pair("]", TokenType::RightBracket)
        );

        std::string source = delimiters.first;
        Lexer lexer(source, "test.lua");
        
        Token token = lexer.NextToken();
        REQUIRE(token.GetType() == delimiters.second);
    }

    SECTION("标点类分隔符应该被正确识别") {
        auto delimiters = GENERATE(
            std::make_pair(";", TokenType::Semicolon),
            std::make_pair(",", TokenType::Comma)
        );

        std::string source = delimiters.first;
        Lexer lexer(source, "test.lua");
        
        Token token = lexer.NextToken();
        REQUIRE(token.GetType() == delimiters.second);
    }
}

/* ========================================================================== */
/* 注释处理契约 */
/* ========================================================================== */

TEST_CASE("Lexer - 单行注释处理契约", "[lexer][contract][comment][line]") {
    SECTION("单行注释应该被忽略") {
        auto testCases = GENERATE(
            "-- this is a comment\nprint('hello')",
            "print('hello') -- end comment",
            "-- comment only\n",
            "-- comment\n-- another comment\nlocal x"
        );

        std::string source = testCases;
        Lexer lexer(source, "test.lua");
        
        Token token = lexer.NextToken();
        // 第一个Token不应该是注释相关的
        REQUIRE((token.GetType() == TokenType::Name ||
                 token.GetType() == TokenType::Local ||
                 token.GetType() == TokenType::EndOfSource));
    }

    SECTION("注释中的特殊字符不应该影响解析") {
        std::string source = "-- comment with \" ' [[ ]] == ~= \nlocal x";
        Lexer lexer(source, "test.lua");
        
        Token token = lexer.NextToken();
        REQUIRE(token.GetType() == TokenType::Local);
    }
}

TEST_CASE("Lexer - 多行注释处理契约", "[lexer][contract][comment][block]") {
    SECTION("多行注释应该被忽略") {
        auto testCases = GENERATE(
            "--[[ comment ]] print('hello')",
            "print('hello') --[[ comment ]]",
            "--[[ \nmulti\nline\ncomment \n]] local x",
            "--[=[ comment with [[ nested ]] ]=] function test()"
        );

        std::string source = testCases;
        Lexer lexer(source, "test.lua");
        
        Token token = lexer.NextToken();
        // 第一个Token不应该是注释相关的
        REQUIRE((token.GetType() == TokenType::Name ||
                 token.GetType() == TokenType::Local ||
                 token.GetType() == TokenType::Function));
    }

    SECTION("嵌套等号的多行注释应该被正确处理") {
        std::string source = "--[==[ comment with [[ and ]] and [=[ sequences ]==] local x";
        Lexer lexer(source, "test.lua");
        
        Token token = lexer.NextToken();
        REQUIRE(token.GetType() == TokenType::Local);
    }

    SECTION("未闭合的多行注释应该报错") {
        std::string source = "--[[ unclosed comment\nprint('hello')";
        Lexer lexer(source, "test.lua");
        
        REQUIRE_THROWS_AS(lexer.NextToken(), LexicalError);
    }
}

/* ========================================================================== */
/* 错误处理契约 */
/* ========================================================================== */

TEST_CASE("Lexer - 词法错误处理契约", "[lexer][contract][error]") {
    SECTION("非法字符应该报错") {
        auto invalidChars = GENERATE(
            "@", "$", "`", "\\", "?", "!"
        );

        std::string source = std::string("local x = ") + invalidChars;
        Lexer lexer(source, "test.lua");
        
        lexer.NextToken(); // local
        lexer.NextToken(); // x
        lexer.NextToken(); // =
        
        REQUIRE_THROWS_AS(lexer.NextToken(), LexicalError);
    }

    SECTION("未闭合的字符串应该报错") {
        auto testCases = GENERATE(
            "\"unclosed string",
            "'unclosed string",
            "\"string with \\",
            "'string with \\"
        );

        std::string source = testCases;
        Lexer lexer(source, "test.lua");
        
        REQUIRE_THROWS_AS(lexer.NextToken(), LexicalError);
    }

    SECTION("非法的数字格式应该报错") {
        auto testCases = GENERATE(
            "0x",          // 空的十六进制
            "1.2.3",       // 多个小数点
            "1e",          // 不完整的科学计数法
            "1e+",         // 不完整的指数
            "0x1.2p"       // 不完整的十六进制指数
        );

        std::string source = testCases;
        Lexer lexer(source, "test.lua");
        
        REQUIRE_THROWS_AS(lexer.NextToken(), LexicalError);
    }

    SECTION("非法的转义序列应该报错") {
        auto testCases = GENERATE(
            "\"\\x\"",     // 非法转义字符
            "\"\\256\"",   // 超出范围的数字转义
            "\"\\400\""    // 超出范围的数字转义
        );

        std::string source = testCases;
        Lexer lexer(source, "test.lua");
        
        REQUIRE_THROWS_AS(lexer.NextToken(), LexicalError);
    }
}

/* ========================================================================== */
/* 位置跟踪契约 */
/* ========================================================================== */

TEST_CASE("Lexer - 行列位置跟踪契约", "[lexer][contract][position]") {
    SECTION("行号应该正确递增") {
        std::string source = "line1\nline2\nline3";
        Lexer lexer(source, "test.lua");
        
        Token token1 = lexer.NextToken();
        REQUIRE(token1.GetLine() == 1);
        
        Token token2 = lexer.NextToken();
        REQUIRE(token2.GetLine() == 2);
        
        Token token3 = lexer.NextToken();
        REQUIRE(token3.GetLine() == 3);
    }

    SECTION("列号应该正确跟踪") {
        std::string source = "a   b    c";
        Lexer lexer(source, "test.lua");
        
        Token token1 = lexer.NextToken();
        REQUIRE(token1.GetColumn() == 1);
        
        Token token2 = lexer.NextToken();
        REQUIRE(token2.GetColumn() == 5);
        
        Token token3 = lexer.NextToken();
        REQUIRE(token3.GetColumn() == 10);
    }

    SECTION("制表符应该正确计算列位置") {
        std::string source = "a\tb\tc";
        Lexer lexer(source, "test.lua");
        
        Token token1 = lexer.NextToken();
        REQUIRE(token1.GetColumn() == 1);
        
        Token token2 = lexer.NextToken();
        REQUIRE(token2.GetColumn() == 9);  // 假设制表符宽度为8
        
        Token token3 = lexer.NextToken();
        REQUIRE(token3.GetColumn() == 17);
    }

    SECTION("多行字符串应该正确跟踪行号") {
        std::string source = "[[\nline1\nline2\n]] local x";
        Lexer lexer(source, "test.lua");
        
        Token token1 = lexer.NextToken();
        REQUIRE(token1.GetType() == TokenType::String);
        REQUIRE(token1.GetLine() == 1);  // 字符串开始位置
        
        Token token2 = lexer.NextToken();
        REQUIRE(token2.GetType() == TokenType::Local);
        REQUIRE(token2.GetLine() == 4);  // 字符串结束后的位置
    }
}

/* ========================================================================== */
/* 性能和边界条件契约 */
/* ========================================================================== */

TEST_CASE("Lexer - 性能和边界条件契约", "[lexer][contract][boundary]") {
    SECTION("超长标识符应该被正确处理") {
        std::string longName(1000, 'a');  // 1000个字符的标识符
        Lexer lexer(longName, "test.lua");
        
        Token token = lexer.NextToken();
        REQUIRE(token.GetType() == TokenType::Name);
        REQUIRE(token.GetString().length() == 1000);
        REQUIRE(token.GetString() == longName);
    }

    SECTION("超长字符串应该被正确处理") {
        std::string longString = "\"" + std::string(10000, 'x') + "\"";
        Lexer lexer(longString, "test.lua");
        
        Token token = lexer.NextToken();
        REQUIRE(token.GetType() == TokenType::String);
        REQUIRE(token.GetString().length() == 10000);
    }

    SECTION("大量Token的序列应该被正确处理") {
        std::string source;
        for (int i = 0; i < 1000; ++i) {
            source += "a" + std::to_string(i) + " ";
        }
        
        Lexer lexer(source, "test.lua");
        
        for (int i = 0; i < 1000; ++i) {
            Token token = lexer.NextToken();
            REQUIRE(token.GetType() == TokenType::Name);
            REQUIRE(token.GetString() == "a" + std::to_string(i));
        }
        
        Token eosToken = lexer.NextToken();
        REQUIRE(eosToken.GetType() == TokenType::EndOfSource);
    }

    SECTION("深度嵌套的多行注释应该被正确处理") {
        std::string source = "--[";
        for (int i = 0; i < 100; ++i) {
            source += "=";
        }
        source += "[ comment ]";
        for (int i = 0; i < 100; ++i) {
            source += "=";
        }
        source += "] local x";
        
        Lexer lexer(source, "test.lua");
        
        Token token = lexer.NextToken();
        REQUIRE(token.GetType() == TokenType::Local);
    }
}