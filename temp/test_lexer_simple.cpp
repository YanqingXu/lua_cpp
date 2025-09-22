#include "src/lexer/lexer.h"
#include "src/lexer/token.h"
#include <iostream>
#include <string>

using namespace lua_cpp;

int main() {
    try {
        // 初始化保留字表
        ReservedWords::Initialize();
        
        // 简单测试
        std::string source = "local x = 42";
        Lexer lexer(source, "test.lua");
        
        std::cout << "Testing Lexer with source: " << source << std::endl;
        
        // 尝试获取token
        Token token1 = lexer.NextToken();
        std::cout << "Token 1: " << token1.ToString() << std::endl;
        
        Token token2 = lexer.NextToken();
        std::cout << "Token 2: " << token2.ToString() << std::endl;
        
        Token token3 = lexer.NextToken();
        std::cout << "Token 3: " << token3.ToString() << std::endl;
        
        Token token4 = lexer.NextToken();
        std::cout << "Token 4: " << token4.ToString() << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}