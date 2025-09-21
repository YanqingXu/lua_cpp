/**
 * @file test_token_simple.cpp
 * @brief 简单的Token系统测试
 * @description 验证Token类基本功能的简单测试
 */

#include <iostream>
#include <cassert>
#include "../src/lexer/token.h"

using namespace lua_cpp;

int main() {
    std::cout << "Testing Token System..." << std::endl;
    
    // 初始化保留字
    ReservedWords::Initialize();
    
    // 测试1: 默认构造
    std::cout << "Test 1: Default constructor..." << std::endl;
    Token defaultToken;
    assert(defaultToken.GetType() == TokenType::EndOfSource);
    assert(defaultToken.GetLine() == 1);
    assert(defaultToken.GetColumn() == 1);
    std::cout << "  Default token: " << defaultToken.ToString() << std::endl;
    
    // 测试2: 数字Token
    std::cout << "Test 2: Number token..." << std::endl;
    Token numberToken = Token::CreateNumber(42.5, 1, 1);
    assert(numberToken.GetType() == TokenType::Number);
    assert(numberToken.GetNumber() == 42.5);
    std::cout << "  Number token: " << numberToken.ToString() << std::endl;
    
    // 测试3: 字符串Token
    std::cout << "Test 3: String token..." << std::endl;
    Token stringToken = Token::CreateString("hello", 1, 5);
    assert(stringToken.GetType() == TokenType::String);
    assert(stringToken.GetString() == "hello");
    std::cout << "  String token: " << stringToken.ToString() << std::endl;
    
    // 测试4: 标识符Token
    std::cout << "Test 4: Name token..." << std::endl;
    Token nameToken = Token::CreateName("variable", 2, 1);
    assert(nameToken.GetType() == TokenType::Name);
    assert(nameToken.GetString() == "variable");
    std::cout << "  Name token: " << nameToken.ToString() << std::endl;
    
    // 测试5: 关键字Token
    std::cout << "Test 5: Keyword token..." << std::endl;
    Token keywordToken = Token::CreateKeyword(TokenType::Function, 3, 1);
    assert(keywordToken.GetType() == TokenType::Function);
    std::cout << "  Keyword token: " << keywordToken.ToString() << std::endl;
    
    // 测试6: 操作符Token
    std::cout << "Test 6: Operator token..." << std::endl;
    Token operatorToken = Token::CreateOperator(TokenType::Plus, 4, 5);
    assert(operatorToken.GetType() == TokenType::Plus);
    std::cout << "  Operator token: " << operatorToken.ToString() << std::endl;
    
    // 测试7: 复制语义
    std::cout << "Test 7: Copy semantics..." << std::endl;
    Token original = Token::CreateString("test", 1, 1);
    Token copied = original;
    assert(copied.GetType() == TokenType::String);
    assert(copied.GetString() == "test");
    std::cout << "  Original: " << original.ToString() << std::endl;
    std::cout << "  Copied: " << copied.ToString() << std::endl;
    
    // 测试8: 移动语义
    std::cout << "Test 8: Move semantics..." << std::endl;
    Token moved = std::move(original);
    assert(moved.GetType() == TokenType::String);
    assert(moved.GetString() == "test");
    std::cout << "  Moved: " << moved.ToString() << std::endl;
    
    // 测试9: 保留字查找
    std::cout << "Test 9: Reserved words..." << std::endl;
    assert(ReservedWords::Lookup("function") == TokenType::Function);
    assert(ReservedWords::Lookup("variable") == TokenType::Name);
    assert(ReservedWords::IsReserved("and"));
    assert(!ReservedWords::IsReserved("variable"));
    std::cout << "  Reserved word lookup works correctly" << std::endl;
    
    // 测试10: Token类型检查
    std::cout << "Test 10: Token type checks..." << std::endl;
    assert(numberToken.IsNumber());
    assert(stringToken.IsString());
    assert(nameToken.IsName());
    assert(keywordToken.IsKeyword());
    assert(operatorToken.IsOperator());
    std::cout << "  Token type checks work correctly" << std::endl;
    
    std::cout << "\nAll tests passed! Token system is working correctly." << std::endl;
    return 0;
}