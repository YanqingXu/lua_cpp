/**
 * @file simple_test_build.cpp  
 * @brief 简化构建测试错误处理系统
 */

#include "../src/lexer/lexer_errors.cpp"  // 直接包含实现
#include "../src/lexer/token.cpp"
#include <iostream>

int main() {
    std::cout << "=== T020 错误处理编译测试 ===" << std::endl;
    
    try {
        // 测试ErrorLocation
        lua_cpp::ErrorLocation location(10, 5, 120, 1, "test.lua", "local x = @");
        std::cout << "ErrorLocation 创建成功" << std::endl;
        
        // 测试LexicalError
        lua_cpp::LexicalError error(lua_cpp::LexicalErrorType::INVALID_CHARACTER, 
                                   "Test error", location);
        std::cout << "LexicalError 创建成功: " << error.what() << std::endl;
        
        // 测试ErrorMessageGenerator
        std::string msg = lua_cpp::ErrorMessageGenerator::GenerateUserMessage(
            lua_cpp::LexicalErrorType::INVALID_CHARACTER, "@");
        std::cout << "错误消息生成: " << msg << std::endl;
        
        // 测试ErrorCollector
        lua_cpp::ErrorCollector collector;
        collector.AddError(error);
        std::cout << "ErrorCollector 添加错误成功, 错误数量: " << collector.GetErrorCount() << std::endl;
        
        // 测试ErrorRecovery
        bool isDelim = lua_cpp::ErrorRecovery::IsDelimiter(' ');
        std::cout << "ErrorRecovery 测试: 空格是分隔符 = " << isDelim << std::endl;
        
        std::cout << "所有基本功能测试成功!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "错误: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "=== T020 错误处理编译测试通过 ===" << std::endl;
    return 0;
}