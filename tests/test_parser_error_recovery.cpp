/**
 * @file test_parser_error_recovery.cpp
 * @brief Parser增强错误恢复测试
 * @description 测试Parser的增强错误恢复功能
 * @date 2025-09-25
 */

#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include "parser/parser.h"
#include "parser/parser_error_recovery.h"
#include "lexer/lexer.h"
#include "core/input_stream.h"

namespace lua_cpp {

/* ========================================================================== */
/* 错误测试用例 */
/* ========================================================================== */

struct ErrorTestCase {
    std::string name;
    std::string source_code;
    std::vector<std::string> expected_errors;
    bool should_recover;
    std::string description;
};

// 定义测试用例
std::vector<ErrorTestCase> GetErrorTestCases() {
    return {
        // 1. 缺失分号测试
        {
            "missing_semicolon",
            "local x = 1\nlocal y = 2",
            {"unexpected token"},
            true,
            "测试缺失分号的错误恢复"
        },
        
        // 2. 不匹配括号测试
        {
            "unmatched_parentheses",
            "local x = (1 + 2\nlocal y = 3",
            {"expected ')' to close '('"},
            true,
            "测试不匹配括号的错误恢复"
        },
        
        // 3. 无效表达式测试
        {
            "invalid_expression", 
            "local x = + * 2",
            {"unexpected token"},
            true,
            "测试无效表达式的错误恢复"
        },
        
        // 4. 缺失end关键字测试
        {
            "missing_end_keyword",
            "if true then\n    print('hello')\nelse\n    print('world')",
            {"expected 'end' to close 'if'"},
            true,
            "测试缺失end关键字的错误恢复"
        },
        
        // 5. 无效函数定义测试
        {
            "invalid_function_definition",
            "function (a, b)\n    return a + b\nend",
            {"expected function name"},
            true,
            "测试无效函数定义的错误恢复"
        },
        
        // 6. 重复的局部变量声明测试
        {
            "duplicate_local_variable",
            "local x = 1\nlocal x = 2",
            {},  // 这实际上在Lua中是合法的
            true,
            "测试重复局部变量声明"
        },
        
        // 7. 表构造语法错误测试
        {
            "table_constructor_error",
            "local t = {a = 1, = 2}",
            {"unexpected token"},
            true,
            "测试表构造语法错误的恢复"
        },
        
        // 8. 循环语法错误测试
        {
            "for_loop_syntax_error",
            "for i = 1, 10\n    print(i)\nend",
            {"expected 'do' after for"},
            true,
            "测试for循环语法错误的恢复"
        }
    };
}

/* ========================================================================== */
/* 测试框架 */
/* ========================================================================== */

class ErrorRecoveryTester {
public:
    ErrorRecoveryTester() : total_tests_(0), passed_tests_(0) {}
    
    void RunAllTests() {
        std::cout << "=== Parser增强错误恢复测试 ===" << std::endl;
        
        auto test_cases = GetErrorTestCases();
        for (const auto& test_case : test_cases) {
            RunSingleTest(test_case);
        }
        
        PrintSummary();
    }
    
private:
    void RunSingleTest(const ErrorTestCase& test_case) {
        total_tests_++;
        
        std::cout << "\n--- 测试: " << test_case.name << " ---" << std::endl;
        std::cout << "描述: " << test_case.description << std::endl;
        std::cout << "源代码:\n" << test_case.source_code << std::endl;
        
        try {
            // 创建Parser配置，启用增强错误恢复
            ParserConfig config;
            config.recover_from_errors = true;
            config.use_enhanced_error_recovery = true;
            config.generate_error_suggestions = true;
            config.max_errors = 10;
            
            // 创建输入流和Lexer
            auto input = std::make_unique<StringInputStream>(test_case.source_code, test_case.name + ".lua");
            auto lexer = std::make_unique<Lexer>(std::move(input));
            
            // 创建Parser
            auto parser = std::make_unique<Parser>(std::move(lexer), config);
            
            // 尝试解析
            std::unique_ptr<Program> ast = nullptr;
            try {
                ast = parser->ParseProgram();
            } catch (const std::exception& e) {
                std::cout << "解析异常: " << e.what() << std::endl;
            }
            
            // 检查错误信息
            auto errors = parser->GetAllErrors();
            std::cout << "检测到 " << errors.size() << " 个错误:" << std::endl;
            
            for (const auto& error : errors) {
                std::cout << "  - " << error.GetMessage() << std::endl;
                std::cout << "    位置: 行 " << error.GetPosition().line 
                         << ", 列 " << error.GetPosition().column << std::endl;
                         
                if (!error.GetSuggestion().empty()) {
                    std::cout << "    建议: " << error.GetSuggestion() << std::endl;
                }
                
                auto suggestions = error.GetSuggestions();
                if (!suggestions.empty()) {
                    std::cout << "    其他建议:" << std::endl;
                    for (const auto& suggestion : suggestions) {
                        std::cout << "      * " << suggestion << std::endl;
                    }
                }
            }
            
            // 验证测试结果
            bool test_passed = true;
            
            // 检查是否按预期发现了错误
            if (!test_case.expected_errors.empty()) {
                bool found_expected_error = false;
                for (const auto& expected : test_case.expected_errors) {
                    for (const auto& error : errors) {
                        if (error.GetMessage().find(expected) != std::string::npos) {
                            found_expected_error = true;
                            break;
                        }
                    }
                    if (found_expected_error) break;
                }
                
                if (!found_expected_error) {
                    std::cout << "❌ 未找到预期的错误信息" << std::endl;
                    test_passed = false;
                } else {
                    std::cout << "✅ 找到了预期的错误信息" << std::endl;
                }
            }
            
            // 检查错误恢复是否成功
            if (test_case.should_recover) {
                if (parser->GetState() == ParserState::Completed || 
                    parser->GetState() == ParserState::Error) {
                    std::cout << "✅ 错误恢复成功" << std::endl;
                } else {
                    std::cout << "❌ 错误恢复失败" << std::endl;
                    test_passed = false;
                }
            }
            
            if (test_passed) {
                passed_tests_++;
                std::cout << "✅ 测试通过" << std::endl;
            } else {
                std::cout << "❌ 测试失败" << std::endl;
            }
            
        } catch (const std::exception& e) {
            std::cout << "❌ 测试异常: " << e.what() << std::endl;
        }
    }
    
    void PrintSummary() {
        std::cout << "\n=== 测试总结 ===" << std::endl;
        std::cout << "总测试数: " << total_tests_ << std::endl;
        std::cout << "通过测试: " << passed_tests_ << std::endl;
        std::cout << "失败测试: " << (total_tests_ - passed_tests_) << std::endl;
        std::cout << "通过率: " << (total_tests_ > 0 ? 
            (100.0 * passed_tests_ / total_tests_) : 0.0) << "%" << std::endl;
    }
    
private:
    int total_tests_;
    int passed_tests_;
};

} // namespace lua_cpp

/* ========================================================================== */
/* 主函数 */
/* ========================================================================== */

int main() {
    try {
        lua_cpp::ErrorRecoveryTester tester;
        tester.RunAllTests();
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "测试程序异常: " << e.what() << std::endl;
        return 1;
    }
}