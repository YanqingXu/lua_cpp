#include "src/lexer/lexer.h"
#include "src/lexer/token.h"
#include <iostream>
#include <string>
#include <vector>

using namespace lua_cpp;

void TestSource(const std::string& source, const std::string& description) {
    std::cout << "\n=== " << description << " ===" << std::endl;
    std::cout << "Source: " << source << std::endl;
    
    try {
        Lexer lexer(source, "test.lua");
        
        std::vector<Token> tokens;
        Token token;
        do {
            token = lexer.NextToken();
            tokens.push_back(token);
            std::cout << "  " << tokens.size() << ": " << token.ToString() << std::endl;
        } while (token.GetType() != TokenType::EndOfSource && tokens.size() < 20);
        
        std::cout << "Total tokens: " << tokens.size() << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

int main() {
    try {
        // 初始化保留字表
        ReservedWords::Initialize();
        
        // 测试各种情况
        TestSource("()", "简单括号");
        TestSource("local x = 42", "基本赋值");
        TestSource("function test(a, b) end", "函数定义");
        TestSource("x = y + z * 2", "数学表达式");
        TestSource("if x > 0 then print(x) end", "条件语句");
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}