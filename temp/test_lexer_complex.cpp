#include "src/lexer/lexer.h"
#include "src/lexer/token.h"
#include <iostream>
#include <string>

using namespace lua_cpp;

int main() {
    try {
        // 初始化保留字表
        ReservedWords::Initialize();
        
        // 测试更复杂的源码
        std::string source = R"(
            local function test(x, y)
                if x > y then
                    return x + y * 2.5
                else
                    return "hello world"
                end
            end
        )";
        
        Lexer lexer(source, "test.lua");
        
        std::cout << "Testing complex Lua source:" << std::endl;
        std::cout << source << std::endl << std::endl;
        
        std::cout << "Tokens:" << std::endl;
        int count = 0;
        while (count < 30) {
            Token token = lexer.NextToken();
            std::cout << "  " << token.ToString() << std::endl;
            
            if (token.GetType() == TokenType::EndOfSource) {
                break;
            }
            count++;
        }
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}