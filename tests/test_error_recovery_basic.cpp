/**
 * @file test_error_recovery_basic.cpp
 * @brief Basic error recovery test (English only)
 * @description Test enhanced error recovery system basic functionality
 * @date 2025-09-25
 */

#include <iostream>
#include <string>
#include <cassert>
#include "parser/parser_error_recovery.h"
#include "core/lua_common.h"

namespace lua_cpp {

void TestEnhancedSyntaxError() {
    std::cout << "Testing EnhancedSyntaxError class..." << std::endl;
    
    // Create error object
    SourcePosition pos{10, 5};
    EnhancedSyntaxError error("Test error", ErrorSeverity::Error, pos, ErrorCategory::Syntax, "Fix suggestion");
    
    // Test basic properties
    assert(std::string(error.what()) == "Test error");
    assert(error.GetSeverity() == ErrorSeverity::Error);
    assert(error.GetCategory() == ErrorCategory::Syntax);
    assert(error.GetSuggestion() == "Fix suggestion");
    assert(error.GetPosition().line == 10);
    assert(error.GetPosition().column == 5);
    
    // Test adding context
    error.AddContext("Line 9: local x = 1");
    error.AddContext("Line 10: local y = ");  // error line
    error.AddContext("Line 11: local z = 3");
    
    assert(error.GetContext().size() == 3);
    
    // Test setting suggestions
    std::vector<std::string> suggestions = {"Check syntax", "Add missing expression", "Reference manual"};
    error.SetSuggestions(suggestions);
    
    assert(error.GetSuggestions().size() == 3);
    assert(error.GetSuggestions()[0] == "Check syntax");
    
    std::cout << "✓ EnhancedSyntaxError test passed" << std::endl;
}

void TestErrorCollector() {
    std::cout << "Testing ErrorCollector class..." << std::endl;
    
    ErrorCollector collector;
    
    // Create test errors
    SourcePosition pos1{5, 10};
    SourcePosition pos2{12, 3};
    SourcePosition pos3{20, 8};
    
    EnhancedSyntaxError error1("Syntax error 1", ErrorSeverity::Error, pos1, ErrorCategory::Syntax);
    EnhancedSyntaxError error2("Syntax error 2", ErrorSeverity::Warning, pos2, ErrorCategory::Lexical);
    EnhancedSyntaxError error3("Syntax error 3", ErrorSeverity::Fatal, pos3, ErrorCategory::Semantic);
    
    // Add errors
    collector.AddError(error1);
    collector.AddError(error2);
    collector.AddError(error3);
    
    // Test error count
    assert(collector.GetErrorCount() == 3);
    assert(collector.GetWarningCount() == 1);
    assert(collector.HasFatalError() == true);
    
    // Test get errors
    auto errors = collector.GetErrors();
    assert(errors.size() == 3);
    
    // Test get errors by severity
    auto fatal_errors = collector.GetErrorsBySeverity(ErrorSeverity::Fatal);
    assert(fatal_errors.size() == 1);
    assert(std::string(fatal_errors[0].what()) == "Syntax error 3");
    
    std::cout << "✓ ErrorCollector test passed" << std::endl;
}

void TestLua51ErrorFormatter() {
    std::cout << "Testing Lua51ErrorFormatter class..." << std::endl;
    
    Lua51ErrorFormatter formatter;
    
    // Create test error
    SourcePosition pos{42, 15};
    EnhancedSyntaxError error("unexpected symbol near '='", ErrorSeverity::Error, pos, ErrorCategory::Syntax);
    error.AddContext("local function test()");
    error.AddContext("    local x =");  // error line
    error.AddContext("end");
    
    // Format error
    std::string formatted = formatter.Format(error);
    
    // Check formatted result contains key information
    assert(formatted.find("42") != std::string::npos);  // line number
    assert(formatted.find("unexpected symbol") != std::string::npos);  // error message
    
    std::cout << "Formatted result:" << std::endl;
    std::cout << formatted << std::endl;
    
    std::cout << "✓ Lua51ErrorFormatter test passed" << std::endl;
}

void TestErrorSuggestionGenerator() {
    std::cout << "Testing ErrorSuggestionGenerator class..." << std::endl;
    
    ErrorSuggestionGenerator generator;
    
    // Create test error (missing semicolon)
    SourcePosition pos{10, 8};
    EnhancedSyntaxError error("unexpected token", ErrorSeverity::Error, pos, ErrorCategory::Syntax);
    
    // Mock current token (simplified handling here)
    Token current_token = Token::CreateIdentifier("local", 10, 1);
    
    // Generate suggestions
    auto suggestions = generator.GenerateSuggestions(error, current_token, nullptr);
    
    // Check if suggestions were generated
    assert(!suggestions.empty());
    
    std::cout << "Generated suggestions:" << std::endl;
    for (const auto& suggestion : suggestions) {
        std::cout << "  - " << suggestion << std::endl;
    }
    
    std::cout << "✓ ErrorSuggestionGenerator test passed" << std::endl;
}

void TestErrorRecoveryEngine() {
    std::cout << "Testing ErrorRecoveryEngine class..." << std::endl;
    
    ErrorRecoveryEngine engine;
    
    // Create test error context
    ErrorContext context;
    context.current_token = Token::CreateSymbol(TokenType::Equal, 5, 10);
    context.position = SourcePosition{5, 10};
    context.recursion_depth = 2;
    context.expression_depth = 1;
    context.parsing_state = ParserState::Parsing;
    
    // Analyze and get recovery actions
    auto actions = engine.AnalyzeAndRecover(context);
    
    // Check if recovery actions were generated
    assert(!actions.empty());
    
    std::cout << "Generated recovery actions:" << std::endl;
    for (const auto& action : actions) {
        std::cout << "  - Action type: ";
        switch (action.type) {
            case RecoveryActionType::SkipToken:
                std::cout << "Skip Token";
                break;
            case RecoveryActionType::InsertToken:
                std::cout << "Insert Token";
                break;
            case RecoveryActionType::SynchronizeToKeyword:
                std::cout << "Synchronize To Keyword";
                break;
            case RecoveryActionType::RestartStatement:
                std::cout << "Restart Statement";
                break;
            case RecoveryActionType::BacktrackAndRetry:
                std::cout << "Backtrack And Retry";
                break;
        }
        std::cout << std::endl;
        
        if (!action.description.empty()) {
            std::cout << "    Description: " << action.description << std::endl;
        }
    }
    
    std::cout << "✓ ErrorRecoveryEngine test passed" << std::endl;
}

} // namespace lua_cpp

int main() {
    try {
        std::cout << "=== Enhanced Error Recovery System Basic Tests ===" << std::endl;
        
        lua_cpp::TestEnhancedSyntaxError();
        lua_cpp::TestErrorCollector();
        lua_cpp::TestLua51ErrorFormatter();
        lua_cpp::TestErrorSuggestionGenerator();
        lua_cpp::TestErrorRecoveryEngine();
        
        std::cout << "\n=== All Tests Passed ===" << std::endl;
        std::cout << "✓ Enhanced error recovery system basic functionality works correctly" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ Unknown test exception" << std::endl;
        return 1;
    }
}