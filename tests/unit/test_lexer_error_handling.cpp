/**
 * @file test_lexer_error_handling.cpp
 * @brief Lexer错误处理测试 - T020 SDD实现
 * @description 测试T020任务的词法分析错误处理功能
 * @author Lua C++ Project
 * @date 2025-09-23
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include "../src/lexer/lexer.h"
#include "../src/lexer/lexer_errors.h"
#include <string>
#include <sstream>

using namespace lua_cpp;

/* ========================================================================== */
/* 错误分类测试 */
/* ========================================================================== */

TEST_CASE("LexicalError - 错误分类系统", "[lexer][error][classification]") {
    SECTION("错误类型枚举完整性") {
        // 验证所有错误类型都有对应的字符串表示
        auto error_types = {
            LexicalErrorType::INVALID_CHARACTER,
            LexicalErrorType::UNTERMINATED_STRING,
            LexicalErrorType::INVALID_ESCAPE_SEQUENCE,
            LexicalErrorType::INCOMPLETE_HEX_NUMBER,
            LexicalErrorType::INCOMPLETE_EXPONENT,
            LexicalErrorType::MULTIPLE_DECIMAL_POINTS,
            LexicalErrorType::UNTERMINATED_LONG_STRING,
            LexicalErrorType::UNTERMINATED_LONG_COMMENT,
            LexicalErrorType::TOKEN_TOO_LONG,
            LexicalErrorType::UNEXPECTED_EOF
        };
        
        for (auto error_type : error_types) {
            std::string type_name = ErrorMessageGenerator::GetErrorTypeName(error_type);
            REQUIRE_FALSE(type_name.empty());
            REQUIRE(type_name != "UNKNOWN");
        }
    }

    SECTION("错误严重性级别") {
        ErrorLocation location(1, 1, 0, 1, "test.lua");
        
        LexicalError warning(LexicalErrorType::MIXED_LINE_ENDINGS, "Mixed line endings", 
                            location, ErrorSeverity::WARNING);
        LexicalError error(LexicalErrorType::INVALID_CHARACTER, "Invalid character", 
                          location, ErrorSeverity::ERROR);
        LexicalError fatal(LexicalErrorType::UNEXPECTED_EOF, "Unexpected EOF", 
                          location, ErrorSeverity::FATAL);
        
        REQUIRE(warning.GetSeverity() == ErrorSeverity::WARNING);
        REQUIRE(error.GetSeverity() == ErrorSeverity::ERROR);
        REQUIRE(fatal.GetSeverity() == ErrorSeverity::FATAL);
    }
}

/* ========================================================================== */
/* 错误信息生成测试 */
/* ========================================================================== */

TEST_CASE("ErrorMessageGenerator - 错误信息生成", "[lexer][error][message]") {
    SECTION("用户友好消息生成") {
        std::string msg1 = ErrorMessageGenerator::GenerateUserMessage(
            LexicalErrorType::INVALID_CHARACTER, "@");
        REQUIRE(msg1.find("Invalid character") != std::string::npos);
        REQUIRE(msg1.find("@") != std::string::npos);
        
        std::string msg2 = ErrorMessageGenerator::GenerateUserMessage(
            LexicalErrorType::UNTERMINATED_STRING);
        REQUIRE(msg2.find("Unterminated string") != std::string::npos);
    }
    
    SECTION("修复建议生成") {
        auto suggestions = ErrorMessageGenerator::GenerateFixSuggestions(
            LexicalErrorType::UNTERMINATED_STRING);
        REQUIRE_FALSE(suggestions.empty());
        REQUIRE(suggestions.size() >= 1);
        
        // 检查建议内容是否相关
        bool has_quote_suggestion = false;
        for (const auto& suggestion : suggestions) {
            if (suggestion.find("quote") != std::string::npos) {
                has_quote_suggestion = true;
                break;
            }
        }
        REQUIRE(has_quote_suggestion);
    }
    
    SECTION("详细错误消息格式") {
        ErrorLocation location(10, 5, 120, 1, "test.lua", "local x = @");
        std::string detailed = ErrorMessageGenerator::GenerateDetailedMessage(
            LexicalErrorType::INVALID_CHARACTER, location, "@");
        
        REQUIRE(detailed.find("test.lua:10:5") != std::string::npos);
        REQUIRE(detailed.find("error:") != std::string::npos);
        REQUIRE(detailed.find("Invalid character") != std::string::npos);
    }
}

/* ========================================================================== */
/* 错误位置信息测试 */
/* ========================================================================== */

TEST_CASE("ErrorLocation - 错误位置信息", "[lexer][error][location]") {
    SECTION("位置信息构造") {
        ErrorLocation location(10, 5, 120, 3, "test.lua", "local x = abc");
        
        REQUIRE(location.line == 10);
        REQUIRE(location.column == 5);
        REQUIRE(location.offset == 120);
        REQUIRE(location.length == 3);
        REQUIRE(location.source_name == "test.lua");
        REQUIRE(location.line_text == "local x = abc");
    }
    
    SECTION("可视化错误指示器") {
        ErrorLocation location(1, 5, 4, 3, "test.lua", "abc def ghi");
        std::string visual = location.GetVisualIndicator();
        
        REQUIRE_FALSE(visual.empty());
        REQUIRE(visual.find("abc def ghi") != std::string::npos);
        REQUIRE(visual.find("^^^") != std::string::npos);
    }
    
    SECTION("制表符处理") {
        ErrorLocation location(1, 9, 8, 1, "test.lua", "abc\tdef\tghi");
        std::string visual = location.GetVisualIndicator();
        
        // 应该正确处理制表符的对齐
        REQUIRE_FALSE(visual.empty());
    }
}

/* ========================================================================== */
/* 错误收集器测试 */
/* ========================================================================== */

TEST_CASE("ErrorCollector - 错误收集", "[lexer][error][collector]") {
    ErrorCollector collector;
    
    SECTION("错误添加和查询") {
        REQUIRE_FALSE(collector.HasErrors());
        REQUIRE(collector.GetErrorCount() == 0);
        
        ErrorLocation location(1, 1, 0, 1, "test.lua");
        LexicalError error1(LexicalErrorType::INVALID_CHARACTER, "Invalid @", location);
        LexicalError error2(LexicalErrorType::UNTERMINATED_STRING, "Unterminated string", location);
        
        collector.AddError(error1);
        collector.AddError(error2);
        
        REQUIRE(collector.HasErrors());
        REQUIRE(collector.GetErrorCount() == 2);
    }
    
    SECTION("按严重性分类") {
        ErrorLocation location(1, 1, 0, 1, "test.lua");
        
        collector.AddError(LexicalErrorType::MIXED_LINE_ENDINGS, "Warning", 
                          location, ErrorSeverity::WARNING);
        collector.AddError(LexicalErrorType::INVALID_CHARACTER, "Error", 
                          location, ErrorSeverity::ERROR);
        collector.AddError(LexicalErrorType::UNEXPECTED_EOF, "Fatal", 
                          location, ErrorSeverity::FATAL);
        
        REQUIRE(collector.GetWarningCount() == 1);
        REQUIRE(collector.GetErrorCount(ErrorSeverity::ERROR) == 1);
        REQUIRE(collector.HasFatalErrors());
    }
    
    SECTION("错误报告生成") {
        ErrorLocation location(1, 1, 0, 1, "test.lua");
        collector.AddError(LexicalErrorType::INVALID_CHARACTER, "Test error", location);
        
        std::string report = collector.GenerateReport();
        REQUIRE_FALSE(report.empty());
        REQUIRE(report.find("ERRORS") != std::string::npos);
        
        std::string summary = collector.GenerateSummary();
        REQUIRE_FALSE(summary.empty());
        REQUIRE(summary.find("1 error") != std::string::npos);
    }
    
    SECTION("最大错误数量限制") {
        collector.SetMaxErrors(2);
        ErrorLocation location(1, 1, 0, 1, "test.lua");
        
        collector.AddError(LexicalErrorType::INVALID_CHARACTER, "Error 1", location);
        collector.AddError(LexicalErrorType::INVALID_CHARACTER, "Error 2", location);
        collector.AddError(LexicalErrorType::INVALID_CHARACTER, "Error 3", location);
        
        REQUIRE(collector.GetErrorCount() == 2); // 应该被限制在2个
    }
}

/* ========================================================================== */
/* 错误恢复策略测试 */
/* ========================================================================== */

TEST_CASE("ErrorRecovery - 错误恢复策略", "[lexer][error][recovery]") {
    SECTION("分隔符检测") {
        REQUIRE(ErrorRecovery::IsDelimiter(' '));
        REQUIRE(ErrorRecovery::IsDelimiter('\t'));
        REQUIRE(ErrorRecovery::IsDelimiter('\n'));
        REQUIRE(ErrorRecovery::IsDelimiter('('));
        REQUIRE(ErrorRecovery::IsDelimiter(')'));
        REQUIRE(ErrorRecovery::IsDelimiter('+'));
        REQUIRE(ErrorRecovery::IsDelimiter('='));
        
        REQUIRE_FALSE(ErrorRecovery::IsDelimiter('a'));
        REQUIRE_FALSE(ErrorRecovery::IsDelimiter('1'));
        REQUIRE_FALSE(ErrorRecovery::IsDelimiter('_'));
    }
    
    SECTION("关键字开始字符检测") {
        REQUIRE(ErrorRecovery::IsKeywordStart('a'));
        REQUIRE(ErrorRecovery::IsKeywordStart('Z'));
        REQUIRE(ErrorRecovery::IsKeywordStart('_'));
        
        REQUIRE_FALSE(ErrorRecovery::IsKeywordStart('1'));
        REQUIRE_FALSE(ErrorRecovery::IsKeywordStart(' '));
        REQUIRE_FALSE(ErrorRecovery::IsKeywordStart('+'));
    }
    
    SECTION("错误恢复执行") {
        std::string test_input = "abc@def ghi";
        size_t pos = 3; // 指向 '@'
        int current_char = test_input[pos];
        
        auto advance = [&]() {
            if (pos < test_input.length() - 1) {
                current_char = test_input[++pos];
            } else {
                current_char = EOZ;
            }
        };
        
        // 测试跳过字符策略
        bool result = ErrorRecovery::ExecuteRecovery(
            RecoveryStrategy::SKIP_CHARACTER, current_char, advance);
        REQUIRE(result);
        REQUIRE(current_char == 'd'); // 应该跳过 '@' 到达 'd'
    }
}

/* ========================================================================== */
/* Lexer错误处理集成测试 */
/* ========================================================================== */

TEST_CASE("Lexer - 错误处理集成", "[lexer][error][integration]") {
    SECTION("错误收集模式") {
        std::string source = "local x = @ + $ - !";
        Lexer lexer(source, "test.lua");
        lexer.SetErrorCollectionMode(true);
        
        // 尝试词法分析包含多个错误的源码
        std::vector<Token> tokens;
        while (!lexer.IsAtEnd() && tokens.size() < 10) {
            try {
                Token token = lexer.NextToken();
                tokens.push_back(token);
                if (token.GetType() == TokenType::EndOfSource) {
                    break;
                }
            } catch (...) {
                break; // 即使在收集模式下也可能抛出异常
            }
        }
        
        // 检查是否收集了错误
        REQUIRE(lexer.HasErrors());
        std::string report = lexer.GetErrorReport();
        REQUIRE_FALSE(report.empty());
    }
    
    SECTION("立即抛出模式") {
        std::string source = "local x = @";
        Lexer lexer(source, "test.lua");
        lexer.SetErrorCollectionMode(false);
        
        lexer.NextToken(); // local
        lexer.NextToken(); // x
        lexer.NextToken(); // =
        
        // 下一个token应该抛出异常
        REQUIRE_THROWS_AS(lexer.NextToken(), LexicalError);
    }
    
    SECTION("错误恢复后继续分析") {
        std::string source = "local @ x = 42";
        Lexer lexer(source, "test.lua");
        lexer.SetErrorCollectionMode(true);
        
        std::vector<Token> tokens;
        while (!lexer.IsAtEnd() && tokens.size() < 10) {
            try {
                Token token = lexer.NextToken();
                tokens.push_back(token);
                if (token.GetType() == TokenType::EndOfSource) {
                    break;
                }
            } catch (...) {
                break;
            }
        }
        
        // 应该能够收集一些有效的Token
        REQUIRE(tokens.size() >= 3); // 至少应该有 local, x, =, 42
        REQUIRE(lexer.HasErrors());
    }
}

/* ========================================================================== */
/* 具体错误场景测试 */
/* ========================================================================== */

TEST_CASE("Lexer - 具体错误场景", "[lexer][error][scenarios]") {
    SECTION("非法字符错误") {
        auto invalid_chars = GENERATE("@", "$", "`", "\\", "?", "!");
        
        std::string source = std::string("local x = ") + invalid_chars;
        Lexer lexer(source, "test.lua");
        lexer.SetErrorCollectionMode(true);
        
        // 分析前面的有效Token
        lexer.NextToken(); // local
        lexer.NextToken(); // x  
        lexer.NextToken(); // =
        
        // 下一个应该遇到错误
        lexer.NextToken(); // 应该遇到非法字符
        
        REQUIRE(lexer.HasErrors());
        const auto& errors = lexer.GetErrorCollector().GetErrors();
        bool found_invalid_char = false;
        for (const auto& error : errors) {
            if (error.GetErrorType() == LexicalErrorType::INVALID_CHARACTER) {
                found_invalid_char = true;
                break;
            }
        }
        REQUIRE(found_invalid_char);
    }
    
    SECTION("未终止字符串错误") {
        auto test_cases = GENERATE(
            "\"unclosed string",
            "'unclosed string",
            "\"string with \\",
            "'string with \\"
        );
        
        std::string source = test_cases;
        Lexer lexer(source, "test.lua");
        lexer.SetErrorCollectionMode(true);
        
        lexer.NextToken(); // 应该遇到错误
        
        REQUIRE(lexer.HasErrors());
        const auto& errors = lexer.GetErrorCollector().GetErrors();
        bool found_unterminated = false;
        for (const auto& error : errors) {
            if (error.GetErrorType() == LexicalErrorType::UNTERMINATED_STRING) {
                found_unterminated = true;
                break;
            }
        }
        REQUIRE(found_unterminated);
    }
    
    SECTION("数字格式错误") {
        auto test_cases = GENERATE(
            "0x",          // 空的十六进制
            "1.2.3",       // 多个小数点
            "1e",          // 不完整的科学计数法
            "1e+",         // 不完整的指数
            "0x1.2p"       // 不完整的十六进制指数
        );
        
        std::string source = test_cases;
        Lexer lexer(source, "test.lua");
        lexer.SetErrorCollectionMode(true);
        
        lexer.NextToken(); // 应该遇到数字格式错误
        
        REQUIRE(lexer.HasErrors());
        const auto& errors = lexer.GetErrorCollector().GetErrors();
        bool found_number_error = false;
        for (const auto& error : errors) {
            LexicalErrorType type = error.GetErrorType();
            if (type == LexicalErrorType::INCOMPLETE_HEX_NUMBER ||
                type == LexicalErrorType::INCOMPLETE_EXPONENT ||
                type == LexicalErrorType::MULTIPLE_DECIMAL_POINTS ||
                type == LexicalErrorType::INVALID_NUMBER_FORMAT) {
                found_number_error = true;
                break;
            }
        }
        REQUIRE(found_number_error);
    }
}

/* ========================================================================== */
/* 性能和边界测试 */
/* ========================================================================== */

TEST_CASE("Lexer - 错误处理性能", "[lexer][error][performance]") {
    SECTION("大量错误处理") {
        std::stringstream source;
        for (int i = 0; i < 100; ++i) {
            source << "@ ";
        }
        
        Lexer lexer(source.str(), "test.lua");
        lexer.SetErrorCollectionMode(true);
        
        // 处理所有Token（包括错误）
        while (!lexer.IsAtEnd()) {
            try {
                Token token = lexer.NextToken();
                if (token.GetType() == TokenType::EndOfSource) {
                    break;
                }
            } catch (...) {
                break;
            }
        }
        
        REQUIRE(lexer.HasErrors());
        // 验证错误收集器能够处理大量错误
        REQUIRE(lexer.GetErrorCollector().GetErrorCount() > 0);
    }
    
    SECTION("错误恢复效率") {
        std::string source = "local @ x $ = 42 ! + @ y";
        Lexer lexer(source, "test.lua");
        lexer.SetErrorCollectionMode(true);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        while (!lexer.IsAtEnd()) {
            try {
                Token token = lexer.NextToken();
                if (token.GetType() == TokenType::EndOfSource) {
                    break;
                }
            } catch (...) {
                break;
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        // 错误恢复应该在合理时间内完成
        REQUIRE(duration.count() < 100); // 少于100ms
        REQUIRE(lexer.HasErrors());
    }
}