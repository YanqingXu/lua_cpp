/**
 * @file test_error_handling_quick.cpp
 * @brief T020错误处理快速验证测试
 * @description 快速验证T020实现的错误处理系统
 */

#include "../src/lexer/lexer.h"
#include "../src/lexer/lexer_errors.h"
#include <iostream>
#include <string>

using namespace lua_cpp;

int main() {
    std::cout << "=== T020 Lexer错误处理系统快速测试 ===" << std::endl;
    
    // 测试1: 基本错误收集
    std::cout << "\n测试1: 基本错误收集模式" << std::endl;
    try {
        std::string source = "local x = @ + $ - !";
        Lexer lexer(source, "test.lua");
        lexer.SetErrorCollectionMode(true);
        
        std::cout << "源码: " << source << std::endl;
        
        while (!lexer.IsAtEnd()) {
            try {
                Token token = lexer.NextToken();
                if (token.GetType() == TokenType::EndOfSource) {
                    break;
                }
                std::cout << "Token: " << static_cast<int>(token.GetType()) << std::endl;
            } catch (...) {
                break;
            }
        }
        
        if (lexer.HasErrors()) {
            std::cout << "收集到错误，生成报告:" << std::endl;
            std::cout << lexer.GetErrorReport() << std::endl;
        } else {
            std::cout << "未收集到错误" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "异常: " << e.what() << std::endl;
    }
    
    // 测试2: 错误消息生成
    std::cout << "\n测试2: 错误消息生成" << std::endl;
    try {
        std::string msg = ErrorMessageGenerator::GenerateUserMessage(
            LexicalErrorType::INVALID_CHARACTER, "@");
        std::cout << "无效字符错误消息: " << msg << std::endl;
        
        auto suggestions = ErrorMessageGenerator::GenerateFixSuggestions(
            LexicalErrorType::UNTERMINATED_STRING);
        std::cout << "未终止字符串修复建议:" << std::endl;
        for (const auto& suggestion : suggestions) {
            std::cout << "  - " << suggestion << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "异常: " << e.what() << std::endl;
    }
    
    // 测试3: 错误位置信息
    std::cout << "\n测试3: 错误位置信息" << std::endl;
    try {
        ErrorLocation location(10, 5, 120, 1, "test.lua", "local x = @");
        std::cout << "位置信息: " << location.line << ":" << location.column << std::endl;
        std::cout << "可视化指示器:" << std::endl;
        std::cout << location.GetVisualIndicator() << std::endl;
    } catch (const std::exception& e) {
        std::cout << "异常: " << e.what() << std::endl;
    }
    
    // 测试4: 错误恢复
    std::cout << "\n测试4: 错误恢复策略" << std::endl;
    try {
        std::cout << "分隔符检测:" << std::endl;
        std::cout << "' ' 是分隔符: " << ErrorRecovery::IsDelimiter(' ') << std::endl;
        std::cout << "'a' 是分隔符: " << ErrorRecovery::IsDelimiter('a') << std::endl;
        
        std::cout << "关键字开始字符检测:" << std::endl;
        std::cout << "'l' 是关键字开始: " << ErrorRecovery::IsKeywordStart('l') << std::endl;
        std::cout << "'1' 是关键字开始: " << ErrorRecovery::IsKeywordStart('1') << std::endl;
    } catch (const std::exception& e) {
        std::cout << "异常: " << e.what() << std::endl;
    }
    
    std::cout << "\n=== T020错误处理系统测试完成 ===" << std::endl;
    return 0;
}