#include "src/lexer/lexer.h"
#include "src/lexer/token.h"
#include <iostream>
#include <string>

using namespace lua_cpp;

int main() {
    try {
        // 初始化保留字表
        ReservedWords::Initialize();
        std::cout << "Reserved words initialized." << std::endl;
        
        // 测试简单的源码
        std::string source = "()";
        std::cout << "Testing source: " << source << std::endl;
        
        Lexer lexer(source, "test.lua");
        std::cout << "Lexer created." << std::endl;
        
        for (int i = 0; i < 3; i++) {
            std::cout << "Getting token " << (i+1) << "..." << std::endl;
            Token token = lexer.NextToken();
            std::cout << "  Token " << (i+1) << ": " << token.ToString() << std::endl;
            
            if (token.GetType() == TokenType::EndOfSource) {
                std::cout << "Reached end of source." << std::endl;
                break;
            }
        }
        
        std::cout << "Test completed successfully." << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}