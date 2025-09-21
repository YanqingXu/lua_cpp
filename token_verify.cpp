#include "src/lexer/token.h"
#include <iostream>
#include <cassert>

using namespace lua_cpp;

void testTokenTypes() {
    std::cout << "测试Token类型...\n";
    
    // 测试TokenType枚举
    assert(static_cast<int>(TokenType::Number) >= 0);
    assert(static_cast<int>(TokenType::String) >= 0);
    assert(static_cast<int>(TokenType::Name) >= 0);
    assert(static_cast<int>(TokenType::Plus) == '+');
    
    std::cout << "✓ TokenType枚举测试通过\n";
}

void testTokenPosition() {
    std::cout << "测试TokenPosition...\n";
    
    TokenPosition pos1;
    assert(pos1.line == 0);
    assert(pos1.column == 0);
    
    TokenPosition pos2(10, 20);
    assert(pos2.line == 10);
    assert(pos2.column == 20);
    
    std::cout << "✓ TokenPosition测试通过\n";
}

void testTokenCreation() {
    std::cout << "测试Token创建...\n";
    
    // 测试数字Token
    auto num_token = Token::CreateNumber(42.5, 1, 1);
    assert(num_token.GetType() == TokenType::Number);
    assert(num_token.GetNumber() == 42.5);
    assert(num_token.GetPosition().line == 1);
    assert(num_token.GetPosition().column == 1);
    
    // 测试字符串Token
    auto str_token = Token::CreateString("hello", 2, 5);
    assert(str_token.GetType() == TokenType::String);
    assert(str_token.GetString() == "hello");
    assert(str_token.GetPosition().line == 2);
    assert(str_token.GetPosition().column == 5);
    
    // 测试名称Token
    auto name_token = Token::CreateName("variable", 3, 10);
    assert(name_token.GetType() == TokenType::Name);
    assert(name_token.GetString() == "variable");
    
    std::cout << "✓ Token创建测试通过\n";
}

void testTokenOperators() {
    std::cout << "测试Token操作符...\n";
    
    auto op_token = Token::CreateOperator(TokenType::Plus, 1, 1);
    assert(op_token.GetType() == TokenType::Plus);
    assert(op_token.IsOperator());
    
    std::cout << "✓ Token操作符测试通过\n";
}

void testReservedWords() {
    std::cout << "测试保留字系统...\n";
    
    // 初始化保留字系统
    ReservedWords::Initialize();
    
    // 测试保留字查找
    assert(ReservedWords::Lookup("if") == TokenType::If);
    assert(ReservedWords::Lookup("then") == TokenType::Then);
    assert(ReservedWords::Lookup("else") == TokenType::Else);
    assert(ReservedWords::Lookup("end") == TokenType::End);
    assert(ReservedWords::Lookup("unknown") == TokenType::Name);
    
    std::cout << "✓ 保留字系统测试通过\n";
}

void testTokenString() {
    std::cout << "测试Token字符串表示...\n";
    
    auto num_token = Token::CreateNumber(123.0, 1, 1);
    std::string str = num_token.ToString();
    std::cout << "数字Token字符串: " << str << "\n";
    
    auto name_token = Token::CreateName("test", 1, 1);
    str = name_token.ToString();
    std::cout << "名称Token字符串: " << str << "\n";
    
    std::cout << "✓ Token字符串表示测试通过\n";
}

int main() {
    try {
        std::cout << "=== Token系统验证测试 ===\n\n";
        
        testTokenTypes();
        testTokenPosition();
        testTokenCreation();
        testTokenOperators();
        testReservedWords();
        testTokenString();
        
        std::cout << "\n🎉 所有测试通过！Token系统实现正确。\n";
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ 测试失败: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "❌ 测试失败: 未知异常\n";
        return 1;
    }
}