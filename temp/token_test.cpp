#include "src/lexer/token.h"
#include <iostream>

using namespace lua_cpp;

int main() {
    try {
        // 测试Token的基本功能
        std::cout << "Token系统测试开始...\n";
        
        // 测试保留字
        ReservedWords::Initialize();
        
        // 测试数字Token
        auto num_token = Token::CreateNumber(42.0, 1, 1);
        std::cout << "数字Token: " << num_token.ToString() << "\n";
        
        // 测试字符串Token  
        auto str_token = Token::CreateString("hello", 1, 10);
        std::cout << "字符串Token: " << str_token.ToString() << "\n";
        
        // 测试标识符Token
        auto id_token = Token::CreateName("variable", 1, 20);
        std::cout << "标识符Token: " << id_token.ToString() << "\n";
        
        // 测试操作符Token
        auto op_token = Token::CreateOperator(TokenType::Plus, 1, 30);
        std::cout << "操作符Token: " << op_token.ToString() << "\n";
        
        // 测试保留字查找
        auto lua_type = ReservedWords::Lookup("if");
        std::cout << "保留字'if'查找结果: " << static_cast<int>(lua_type) << "\n";
        
        std::cout << "Token系统测试成功完成！\n";
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "测试失败: " << e.what() << "\n";
        return 1;
    }
}