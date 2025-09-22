/**
 * @file token.h
 * @brief Lua词法分析Token定义
 * @description 基于Lua 5.1.5源码的Token类型和结构定义，提供完整的词法单元表示
 * @date 2025-09-20
 */

#ifndef LUA_TOKEN_H
#define LUA_TOKEN_H

#include "../core/lua_common.h"
#include <string>
#include <variant>
#include <memory>
#include <vector>
#include <unordered_map>

namespace lua_cpp {

/* ========================================================================== */
/* Token类型枚举定义 */
/* ========================================================================== */

/**
 * @brief Token类型枚举
 * @description 基于Lua 5.1.5的RESERVED枚举，包含所有词法单元类型
 * 单字符Token使用ASCII值，多字符Token从257开始
 */
enum class TokenType : int {
    // 单字符Token (使用ASCII值)
    Plus = '+',              // +
    Minus = '-',             // -
    Multiply = '*',          // *
    Divide = '/',            // /
    Modulo = '%',            // %
    Power = '^',             // ^
    Length = '#',            // #
    Less = '<',              // <
    Greater = '>',           // >
    Assign = '=',            // =
    LeftParen = '(',         // (
    RightParen = ')',        // )
    LeftBrace = '{',         // {
    RightBrace = '}',        // }
    LeftBracket = '[',       // [
    RightBracket = ']',      // ]
    Semicolon = ';',         // ;
    Comma = ',',             // ,
    Dot = '.',               // .

    // 保留字Token (从257开始，按字母顺序)
    And = 257,               // and
    Break,                   // break
    Do,                      // do
    Else,                    // else
    ElseIf,                  // elseif
    End,                     // end
    False,                   // false
    For,                     // for
    Function,                // function
    If,                      // if
    In,                      // in
    Local,                   // local
    Nil,                     // nil
    Not,                     // not
    Or,                      // or
    Repeat,                  // repeat
    Return,                  // return
    Then,                    // then
    True,                    // true
    Until,                   // until
    While,                   // while

    // 多字符操作符Token
    Concat,                  // ..
    Dots,                    // ...
    Equal,                   // ==
    GreaterEqual,            // >=
    LessEqual,               // <=
    NotEqual,                // ~=

    // 字面量Token
    Number,                  // 数字字面量
    String,                  // 字符串字面量
    Name,                    // 标识符

    // 特殊Token
    EndOfSource              // 文件结束
};

/**
 * @brief 获取Token类型的字符串表示
 * @param type Token类型
 * @return Token类型的字符串名称
 */
std::string_view GetTokenTypeName(TokenType type);

/**
 * @brief 判断是否为保留字Token
 * @param type Token类型
 * @return 如果是保留字则返回true
 */
constexpr bool IsReservedWord(TokenType type) {
    return static_cast<int>(type) >= static_cast<int>(TokenType::And) &&
           static_cast<int>(type) <= static_cast<int>(TokenType::While);
}

/**
 * @brief 判断是否为操作符Token
 * @param type Token类型
 * @return 如果是操作符则返回true
 */
constexpr bool IsOperator(TokenType type) {
    // 明确列出操作符，避免与分隔符混淆
    int t = static_cast<int>(type);
    bool is_single_char_operator = (t == '+' || t == '-' || t == '*' || t == '/' || 
                                  t == '%' || t == '^' || t == '#' || t == '<' || 
                                  t == '>' || t == '=');
    
    bool is_multi_char_operator = (static_cast<int>(type) >= static_cast<int>(TokenType::Concat) &&
                                 static_cast<int>(type) <= static_cast<int>(TokenType::NotEqual));
    
    return is_single_char_operator || is_multi_char_operator;
}

/**
 * @brief 判断是否为分隔符Token
 * @param type Token类型
 * @return 如果是分隔符则返回true
 */
constexpr bool IsDelimiter(TokenType type) {
    return type == TokenType::LeftParen || type == TokenType::RightParen ||
           type == TokenType::LeftBrace || type == TokenType::RightBrace ||
           type == TokenType::LeftBracket || type == TokenType::RightBracket ||
           type == TokenType::Semicolon || type == TokenType::Comma ||
           type == TokenType::Dot;
}

/* ========================================================================== */
/* Token语义值定义 */
/* ========================================================================== */

/**
 * @brief Token语义值联合
 * @description 对应Lua源码中的SemInfo联合体，存储Token的具体值
 */
using TokenValue = std::variant<
    std::monostate,          // 无值 (用于操作符、关键字等)
    double,                  // 数字值 (对应lua_Number)
    std::string              // 字符串值 (对应TString*)
>;

/* ========================================================================== */
/* Token位置信息定义 */
/* ========================================================================== */

/**
 * @brief Token位置信息
 * @description 记录Token在源码中的位置，用于错误报告和调试
 */
struct TokenPosition {
    Size line;               // 行号 (从1开始)
    Size column;             // 列号 (从1开始)
    Size offset;             // 字符偏移 (从0开始)
    std::string_view source; // 源文件名

    TokenPosition() : line(1), column(1), offset(0) {}
    
    TokenPosition(Size line, Size column, Size offset = 0, std::string_view source = "")
        : line(line), column(column), offset(offset), source(source) {}

    bool operator==(const TokenPosition& other) const {
        return line == other.line && column == other.column && 
               offset == other.offset && source == other.source;
    }

    bool operator!=(const TokenPosition& other) const {
        return !(*this == other);
    }
};

/* ========================================================================== */
/* Token主类定义 */
/* ========================================================================== */

/**
 * @brief Lua词法Token类
 * @description 对应Lua源码中的Token结构体，包含类型、值和位置信息
 */
class Token {
public:
    /* 构造函数和析构函数 */
    Token();
    Token(TokenType type, const TokenPosition& position = TokenPosition{});
    Token(TokenType type, const TokenValue& value, const TokenPosition& position = TokenPosition{});
    Token(const Token& other);
    Token(Token&& other) noexcept;
    ~Token() = default;

    /* 赋值操作符 */
    Token& operator=(const Token& other);
    Token& operator=(Token&& other) noexcept;

    /* 比较操作符 */
    bool operator==(const Token& other) const;
    bool operator!=(const Token& other) const;

    /* 静态工厂方法 */
    static Token CreateEndOfSource(const TokenPosition& position = TokenPosition{});
    static Token CreateNumber(double value, Size line, Size column);
    static Token CreateString(const std::string& value, Size line, Size column);
    static Token CreateName(const std::string& value, Size line, Size column);
    static Token CreateKeyword(TokenType keyword, Size line, Size column);
    static Token CreateOperator(TokenType op, Size line, Size column);
    static Token CreateDelimiter(TokenType delimiter, Size line, Size column);

    /* 访问器方法 */
    TokenType GetType() const { return type_; }
    const TokenValue& GetValue() const { return value_; }
    const TokenPosition& GetPosition() const { return position_; }

    /* 便利访问器 */
    Size GetLine() const { return position_.line; }
    Size GetColumn() const { return position_.column; }
    Size GetOffset() const { return position_.offset; }
    std::string_view GetSource() const { return position_.source; }

    /* 类型判断方法 */
    bool IsEndOfSource() const { return type_ == TokenType::EndOfSource; }
    bool IsNumber() const { return type_ == TokenType::Number; }
    bool IsString() const { return type_ == TokenType::String; }
    bool IsName() const { return type_ == TokenType::Name; }
    bool IsKeyword() const { return IsReservedWord(type_); }
    bool IsOperator() const { return lua_cpp::IsOperator(type_); }
    bool IsDelimiter() const { return lua_cpp::IsDelimiter(type_); }

    /* 值获取方法 */
    double GetNumber() const;
    const std::string& GetString() const;

    /* 调试和显示方法 */
    std::string ToString() const;
    std::string GetLocationString() const;

private:
    TokenType type_;          // Token类型
    TokenValue value_;        // Token值
    TokenPosition position_;  // Token位置

    /* 验证方法 */
    void ValidateTypeAndValue() const;
};

/* ========================================================================== */
/* 保留字查找表 */
/* ========================================================================== */

/**
 * @brief 保留字查找类
 * @description 提供高效的保留字识别功能
 */
class ReservedWords {
public:
    /**
     * @brief 初始化保留字表
     * @description 应该在程序启动时调用一次
     */
    static void Initialize();

    /**
     * @brief 查找保留字
     * @param name 要查找的名称
     * @return 如果是保留字则返回对应的TokenType，否则返回TokenType::Name
     */
    static TokenType Lookup(const std::string& name);

    /**
     * @brief 判断是否为保留字
     * @param name 要判断的名称
     * @return 如果是保留字则返回true
     */
    static bool IsReserved(const std::string& name);

    /**
     * @brief 获取所有保留字
     * @return 保留字列表
     */
    static const std::vector<std::string>& GetAllReservedWords();

private:
    static std::unordered_map<std::string, TokenType> reserved_map_;
    static std::vector<std::string> reserved_list_;
    static bool initialized_;
};

/* ========================================================================== */
/* 常量定义 */
/* ========================================================================== */

// Token类型数量
constexpr Size TOKEN_TYPE_COUNT = static_cast<Size>(TokenType::EndOfSource) - 
                                   static_cast<Size>(TokenType::And) + 20; // 近似值

// 保留字数量
constexpr Size RESERVED_WORD_COUNT = static_cast<Size>(TokenType::While) - 
                                      static_cast<Size>(TokenType::And) + 1;

// 第一个保留字的值
constexpr int FIRST_RESERVED = static_cast<int>(TokenType::And);

} // namespace lua_cpp

#endif // LUA_TOKEN_H