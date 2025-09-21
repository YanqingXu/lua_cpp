/**
 * @file token.cpp
 * @brief Token类型系统实现 - Lua C++解释器的词法单元
 * @description 实现Token类和相关工具函数
 * @author Lua C++ Project
 * @date 2025-09-21
 */

#include "token.h"
#include <stdexcept>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <cassert>

namespace lua_cpp {

/* ========================================================================== */
/* 静态数据初始化 */
/* ========================================================================== */

// 保留字映射表
std::unordered_map<std::string, TokenType> ReservedWords::reserved_map_;
std::vector<std::string> ReservedWords::reserved_list_;
bool ReservedWords::initialized_ = false;

/* ========================================================================== */
/* Token类实现 */
/* ========================================================================== */

Token::Token() : type_(TokenType::EndOfSource), value_(std::monostate{}), position_() {
}

Token::Token(TokenType type, const TokenPosition& position) 
    : type_(type), value_(std::monostate{}), position_(position) {
    ValidateTypeAndValue();
}

Token::Token(TokenType type, const TokenValue& value, const TokenPosition& position)
    : type_(type), value_(value), position_(position) {
    ValidateTypeAndValue();
}

Token::Token(const Token& other) 
    : type_(other.type_), value_(other.value_), position_(other.position_) {
}

Token::Token(Token&& other) noexcept
    : type_(other.type_), value_(std::move(other.value_)), position_(other.position_) {
}

Token& Token::operator=(const Token& other) {
    if (this != &other) {
        type_ = other.type_;
        value_ = other.value_;
        position_ = other.position_;
    }
    return *this;
}

Token& Token::operator=(Token&& other) noexcept {
    if (this != &other) {
        type_ = other.type_;
        value_ = std::move(other.value_);
        position_ = other.position_;
    }
    return *this;
}

bool Token::operator==(const Token& other) const {
    return type_ == other.type_ && value_ == other.value_;
}

bool Token::operator!=(const Token& other) const {
    return !(*this == other);
}

/* ========================================================================== */
/* 静态工厂方法实现 */
/* ========================================================================== */

Token Token::CreateEndOfSource(const TokenPosition& position) {
    return Token(TokenType::EndOfSource, std::monostate{}, position);
}

Token Token::CreateNumber(double value, Size line, Size column) {
    TokenPosition pos(line, column);
    return Token(TokenType::Number, value, pos);
}

Token Token::CreateString(const std::string& value, Size line, Size column) {
    TokenPosition pos(line, column);
    return Token(TokenType::String, value, pos);
}

Token Token::CreateName(const std::string& value, Size line, Size column) {
    TokenPosition pos(line, column);
    return Token(TokenType::Name, value, pos);
}

Token Token::CreateKeyword(TokenType keyword, Size line, Size column) {
    assert(IsReservedWord(keyword) && "Invalid keyword type");
    TokenPosition pos(line, column);
    return Token(keyword, std::monostate{}, pos);
}

Token Token::CreateOperator(TokenType op, Size line, Size column) {
    assert(lua_cpp::IsOperator(op) && "Invalid operator type");
    TokenPosition pos(line, column);
    return Token(op, std::monostate{}, pos);
}

Token Token::CreateDelimiter(TokenType delimiter, Size line, Size column) {
    assert(lua_cpp::IsDelimiter(delimiter) && "Invalid delimiter type");
    TokenPosition pos(line, column);
    return Token(delimiter, std::monostate{}, pos);
}

/* ========================================================================== */
/* 值获取方法实现 */
/* ========================================================================== */

double Token::GetNumber() const {
    if (type_ != TokenType::Number) {
        throw std::runtime_error("Token is not a number");
    }
    
    if (std::holds_alternative<double>(value_)) {
        return std::get<double>(value_);
    }
    
    throw std::runtime_error("Token has invalid number value");
}

const std::string& Token::GetString() const {
    if (type_ != TokenType::String && type_ != TokenType::Name) {
        throw std::runtime_error("Token is not a string or name");
    }
    
    if (std::holds_alternative<std::string>(value_)) {
        return std::get<std::string>(value_);
    }
    
    throw std::runtime_error("Token has invalid string value");
}

/* ========================================================================== */
/* 调试和显示方法实现 */
/* ========================================================================== */

std::string Token::ToString() const {
    std::ostringstream oss;
    oss << GetTokenTypeName(type_);
    
    if (std::holds_alternative<double>(value_)) {
        oss << "(" << std::get<double>(value_) << ")";
    } else if (std::holds_alternative<std::string>(value_)) {
        oss << "(\"" << std::get<std::string>(value_) << "\")";
    }
    
    return oss.str();
}

std::string Token::GetLocationString() const {
    std::ostringstream oss;
    oss << position_.line << ":" << position_.column;
    if (!position_.source.empty()) {
        oss << " (" << position_.source << ")";
    }
    return oss.str();
}

/* ========================================================================== */
/* 验证方法实现 */
/* ========================================================================== */

void Token::ValidateTypeAndValue() const {
    switch (type_) {
        case TokenType::Number:
            if (!std::holds_alternative<double>(value_)) {
                throw std::invalid_argument("Number token must have double value");
            }
            break;
            
        case TokenType::String:
        case TokenType::Name:
            if (!std::holds_alternative<std::string>(value_)) {
                throw std::invalid_argument("String/Name token must have string value");
            }
            break;
            
        default:
            // 其他类型通常不需要值，或者值是可选的
            break;
    }
}

/* ========================================================================== */
/* 全局工具函数实现 */
/* ========================================================================== */

std::string_view GetTokenTypeName(TokenType type) {
    switch (type) {
        // 单字符Token
        case TokenType::Plus: return "Plus";
        case TokenType::Minus: return "Minus";
        case TokenType::Multiply: return "Multiply";
        case TokenType::Divide: return "Divide";
        case TokenType::Modulo: return "Modulo";
        case TokenType::Power: return "Power";
        case TokenType::Length: return "Length";
        case TokenType::Less: return "Less";
        case TokenType::Greater: return "Greater";
        case TokenType::Assign: return "Assign";
        case TokenType::LeftParen: return "LeftParen";
        case TokenType::RightParen: return "RightParen";
        case TokenType::LeftBrace: return "LeftBrace";
        case TokenType::RightBrace: return "RightBrace";
        case TokenType::LeftBracket: return "LeftBracket";
        case TokenType::RightBracket: return "RightBracket";
        case TokenType::Semicolon: return "Semicolon";
        case TokenType::Comma: return "Comma";
        case TokenType::Dot: return "Dot";
        
        // 保留字
        case TokenType::And: return "And";
        case TokenType::Break: return "Break";
        case TokenType::Do: return "Do";
        case TokenType::Else: return "Else";
        case TokenType::ElseIf: return "ElseIf";
        case TokenType::End: return "End";
        case TokenType::False: return "False";
        case TokenType::For: return "For";
        case TokenType::Function: return "Function";
        case TokenType::If: return "If";
        case TokenType::In: return "In";
        case TokenType::Local: return "Local";
        case TokenType::Nil: return "Nil";
        case TokenType::Not: return "Not";
        case TokenType::Or: return "Or";
        case TokenType::Repeat: return "Repeat";
        case TokenType::Return: return "Return";
        case TokenType::Then: return "Then";
        case TokenType::True: return "True";
        case TokenType::Until: return "Until";
        case TokenType::While: return "While";
        
        // 多字符操作符
        case TokenType::Concat: return "Concat";
        case TokenType::Dots: return "Dots";
        case TokenType::Equal: return "Equal";
        case TokenType::GreaterEqual: return "GreaterEqual";
        case TokenType::LessEqual: return "LessEqual";
        case TokenType::NotEqual: return "NotEqual";
        
        // 字面量
        case TokenType::Number: return "Number";
        case TokenType::String: return "String";
        case TokenType::Name: return "Name";
        
        // 特殊
        case TokenType::EndOfSource: return "EndOfSource";
        
        default: return "Unknown";
    }
}

/* ========================================================================== */
/* ReservedWords类实现 */
/* ========================================================================== */

void ReservedWords::Initialize() {
    if (initialized_) {
        return;
    }
    
    // 初始化保留字映射表
    reserved_map_ = {
        {"and", TokenType::And},
        {"break", TokenType::Break},
        {"do", TokenType::Do},
        {"else", TokenType::Else},
        {"elseif", TokenType::ElseIf},
        {"end", TokenType::End},
        {"false", TokenType::False},
        {"for", TokenType::For},
        {"function", TokenType::Function},
        {"if", TokenType::If},
        {"in", TokenType::In},
        {"local", TokenType::Local},
        {"nil", TokenType::Nil},
        {"not", TokenType::Not},
        {"or", TokenType::Or},
        {"repeat", TokenType::Repeat},
        {"return", TokenType::Return},
        {"then", TokenType::Then},
        {"true", TokenType::True},
        {"until", TokenType::Until},
        {"while", TokenType::While}
    };
    
    // 初始化保留字列表
    reserved_list_.reserve(reserved_map_.size());
    for (const auto& pair : reserved_map_) {
        reserved_list_.push_back(pair.first);
    }
    
    initialized_ = true;
}

TokenType ReservedWords::Lookup(const std::string& name) {
    if (!initialized_) {
        Initialize();
    }
    
    auto it = reserved_map_.find(name);
    return (it != reserved_map_.end()) ? it->second : TokenType::Name;
}

bool ReservedWords::IsReserved(const std::string& name) {
    return Lookup(name) != TokenType::Name;
}

const std::vector<std::string>& ReservedWords::GetAllReservedWords() {
    if (!initialized_) {
        Initialize();
    }
    return reserved_list_;
}

} // namespace lua_cpp