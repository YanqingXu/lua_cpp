#include "src/lexer/lexer.h"
#include "src/lexer/token.h"
#include <iostream>
#include <string>

using namespace lua_cpp;

int main() {
    try {
        // 初始化保留字表
        ReservedWords::Initialize();
        
        // 测试简单源码
        std::string source = "local x = 42";
        
        Lexer lexer(source, "test.lua");
        
        std::cout << "Testing source: " << source << std::endl;
        std::cout << "IsAtEnd(): " << lexer.IsAtEnd() << std::endl;
        
        std::cout << "Tokens:" << std::endl;
        for (int i = 0; i < 10; i++) {
            Token token = lexer.NextToken();
            std::cout << "  " << i+1 << ": " << token.ToString() << std::endl;
            
            if (token.GetType() == TokenType::EndOfSource) {
                std::cout << "  Reached end of source" << std::endl;
                break;
            }
        }
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}