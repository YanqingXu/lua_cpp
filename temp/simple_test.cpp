#include "src/lexer/token.h"
#include <iostream>

using namespace lua_cpp;

int main() {
    try {
        std::cout << "开始Token基础测试...\n";
        
        // 测试TokenType枚举
        TokenType type = TokenType::Number;
        std::cout << "TokenType::Number = " << static_cast<int>(type) << "\n";
        
        // 测试TokenPosition
        TokenPosition pos(1, 1);
        std::cout << "TokenPosition创建成功\n";
        
        // 测试简单Token创建
        auto token = Token::CreateNumber(3.14, 1, 1);
        std::cout << "数字Token创建成功\n";
        
        std::cout << "基础测试完成！\n";
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "测试失败: " << e.what() << "\n";
        return 1;
    }
}