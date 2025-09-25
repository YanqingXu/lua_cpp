/**
 * @file test_error_recovery_simple.cpp
 * @brief 简化的错误恢复测试
 * @description 测试增强错误恢复系统的基础功能
 * @date 2025-09-25
 */

#include <iostream>
#include <string>
#include <cassert>
#include "parser/parser_error_recovery.h"
#include "core/lua_common.h"

namespace lua_cpp {

/* ========================================================================== */
/* 基础组件测试 */
/* ========================================================================== */

void TestEnhancedSyntaxError() {
    std::cout << "测试EnhancedSyntaxError类..." << std::endl;
    
    // 创建错误对象
    SourcePosition pos{10, 5};
    EnhancedSyntaxError error("测试错误", ErrorSeverity::Error, pos, ErrorCategory::Syntax, "建议修复");
    
    // 测试基本属性
    assert(std::string(error.what()) == "测试错误");
    assert(error.GetSeverity() == ErrorSeverity::Error);
    assert(error.GetCategory() == ErrorCategory::Syntax);
    assert(error.GetSuggestion() == "建议修复");
    assert(error.GetPosition().line == 10);
    assert(error.GetPosition().column == 5);
    
    // 测试添加上下文
    error.AddContext("第9行: local x = 1");
    error.AddContext("第10行: local y = ");  // 错误行
    error.AddContext("第11行: local z = 3");
    
    assert(error.GetContext().size() == 3);
    
    // 测试设置建议
    std::vector<std::string> suggestions = {"检查语法", "添加缺失的表达式", "参考手册"};
    error.SetSuggestions(suggestions);
    
    assert(error.GetSuggestions().size() == 3);
    assert(error.GetSuggestions()[0] == "检查语法");
    
    std::cout << "✅ EnhancedSyntaxError测试通过" << std::endl;
}

void TestErrorCollector() {
    std::cout << "测试ErrorCollector类..." << std::endl;
    
    ErrorCollector collector;
    
    // 创建几个测试错误
    SourcePosition pos1{5, 10};
    SourcePosition pos2{12, 3};
    SourcePosition pos3{20, 8};
    
    EnhancedSyntaxError error1("语法错误1", ErrorSeverity::Error, pos1, ErrorCategory::Syntax);
    EnhancedSyntaxError error2("语法错误2", ErrorSeverity::Warning, pos2, ErrorCategory::Lexical);
    EnhancedSyntaxError error3("语法错误3", ErrorSeverity::Fatal, pos3, ErrorCategory::Semantic);
    
    // 添加错误
    collector.AddError(error1);
    collector.AddError(error2);
    collector.AddError(error3);
    
    // 测试错误计数
    assert(collector.GetErrorCount() == 3);
    assert(collector.GetWarningCount() == 1);
    assert(collector.HasFatalError() == true);
    
    // 测试获取错误
    auto errors = collector.GetErrors();
    assert(errors.size() == 3);
    
    // 测试按严重性获取错误
    auto fatal_errors = collector.GetErrorsBySeverity(ErrorSeverity::Fatal);
    assert(fatal_errors.size() == 1);
    assert(fatal_errors[0].GetMessage() == "语法错误3");
    
    std::cout << "✅ ErrorCollector测试通过" << std::endl;
}

void TestLua51ErrorFormatter() {
    std::cout << "测试Lua51ErrorFormatter类..." << std::endl;
    
    Lua51ErrorFormatter formatter;
    
    // 创建测试错误
    SourcePosition pos{42, 15};
    EnhancedSyntaxError error("unexpected symbol near '='", ErrorSeverity::Error, pos, ErrorCategory::Syntax);
    error.AddContext("local function test()");
    error.AddContext("    local x =");  // 错误行
    error.AddContext("end");
    
    // 格式化错误
    std::string formatted = formatter.Format(error);
    
    // 检查格式化结果包含关键信息
    assert(formatted.find("42") != std::string::npos);  // 行号
    assert(formatted.find("unexpected symbol") != std::string::npos);  // 错误消息
    
    std::cout << "格式化结果:" << std::endl;
    std::cout << formatted << std::endl;
    
    std::cout << "✅ Lua51ErrorFormatter测试通过" << std::endl;
}

void TestErrorSuggestionGenerator() {
    std::cout << "测试ErrorSuggestionGenerator类..." << std::endl;
    
    ErrorSuggestionGenerator generator;
    
    // 创建测试错误（缺失分号）
    SourcePosition pos{10, 8};
    EnhancedSyntaxError error("unexpected token", ErrorSeverity::Error, pos, ErrorCategory::Syntax);
    
    // 模拟当前token（这里简化处理）
    Token current_token = Token::CreateIdentifier("local", 10, 1);
    
    // 生成建议
    auto suggestions = generator.GenerateSuggestions(error, current_token, nullptr);
    
    // 检查是否生成了建议
    assert(!suggestions.empty());
    
    std::cout << "生成的建议:" << std::endl;
    for (const auto& suggestion : suggestions) {
        std::cout << "  - " << suggestion << std::endl;
    }
    
    std::cout << "✅ ErrorSuggestionGenerator测试通过" << std::endl;
}

void TestErrorRecoveryEngine() {
    std::cout << "测试ErrorRecoveryEngine类..." << std::endl;
    
    ErrorRecoveryEngine engine;
    
    // 创建测试错误上下文
    ErrorContext context;
    context.current_token = Token::CreateSymbol(TokenType::Equal, 5, 10);
    context.position = SourcePosition{5, 10};
    context.recursion_depth = 2;
    context.expression_depth = 1;
    context.parsing_state = ParserState::Parsing;
    
    // 分析并获取恢复动作
    auto actions = engine.AnalyzeAndRecover(context);
    
    // 检查是否生成了恢复动作
    assert(!actions.empty());
    
    std::cout << "生成的恢复动作:" << std::endl;
    for (const auto& action : actions) {
        std::cout << "  - 动作类型: ";
        switch (action.type) {
            case RecoveryActionType::SkipToken:
                std::cout << "跳过Token";
                break;
            case RecoveryActionType::InsertToken:
                std::cout << "插入Token";
                break;
            case RecoveryActionType::SynchronizeToKeyword:
                std::cout << "同步到关键字";
                break;
            case RecoveryActionType::RestartStatement:
                std::cout << "重新开始语句";
                break;
            case RecoveryActionType::BacktrackAndRetry:
                std::cout << "回溯并重试";
                break;
        }
        std::cout << std::endl;
        
        if (!action.description.empty()) {
            std::cout << "    描述: " << action.description << std::endl;
        }
    }
    
    std::cout << "✅ ErrorRecoveryEngine测试通过" << std::endl;
}

} // namespace lua_cpp

/* ========================================================================== */
/* 主函数 */
/* ========================================================================== */

int main() {
    try {
        std::cout << "=== 增强错误恢复系统基础测试 ===" << std::endl;
        
        lua_cpp::TestEnhancedSyntaxError();
        lua_cpp::TestErrorCollector();
        lua_cpp::TestLua51ErrorFormatter();
        lua_cpp::TestErrorSuggestionGenerator();
        lua_cpp::TestErrorRecoveryEngine();
        
        std::cout << "\n=== 所有测试通过 ===" << std::endl;
        std::cout << "✅ 增强错误恢复系统基础功能正常工作" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ 测试失败: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ 未知测试异常" << std::endl;
        return 1;
    }
}