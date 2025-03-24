#pragma once

#include "types.h"
#include <string_view>
#include <vector>
#include <unordered_map>

namespace LuaCore {

/**
 * @brief 表示Lua代码中的标记（token）类型
 */
enum class TokenType {
    // 特殊标记
    EndOfFile,  // 文件结束
    Error,      // 错误标记
    
    // 标识符和字面量
    Identifier, // 标识符
    Number,     // 数字字面量
    String,     // 字符串字面量
    
    // 关键字
    And,        // and
    Break,      // break
    Do,         // do
    Else,       // else
    Elseif,     // elseif
    End,        // end
    False,      // false
    For,        // for
    Function,   // function
    If,         // if
    In,         // in
    Local,      // local
    Nil,        // nil
    Not,        // not
    Or,         // or
    Repeat,     // repeat
    Return,     // return
    Then,       // then
    True,       // true
    Until,      // until
    While,      // while
    
    // 运算符
    Plus,       // +
    Minus,      // -
    Star,       // *
    Slash,      // /
    Percent,    // %
    Caret,      // ^
    Hash,       // #
    Equal,      // ==
    NotEqual,   // ~=
    LessThan,   // <
    LessEqual,  // <=
    GreaterThan,// >
    GreaterEqual,// >=
    Assign,     // =
    Concat,     // ..
    Dot,        // .
    Comma,      // ,
    Semicolon,  // ;
    Colon,      // :
    DoubleColon,// ::
    
    // 括号
    LeftParen,  // (
    RightParen, // )
    LeftBrace,  // {
    RightBrace, // }
    LeftBracket,// [
    RightBracket,// ]
};

/**
 * @brief 表示Lua代码中的一个标记（token）
 */
struct Token {
    TokenType type;      // 标记类型
    Str lexeme;          // 标记的实际文本
    int line;            // 标记所在的行号
    int column;          // 标记所在的列号
    
    // 对于数字字面量
    double numberValue = 0.0;
    
    // 对于字符串字面量（已处理转义序列）
    Str stringValue;
};

/**
 * @brief Lua代码的词法分析器
 *
 * Lexer类负责将Lua源代码转换为一系列标记（tokens），
 * 供Parser使用进行语法分析。
 */
class Lexer {
public:
    /**
     * 构造函数
     * @param source 源代码
     */
    explicit Lexer(const Str& source);
    
    /**
     * 获取下一个标记
     * @return 标记
     */
    Token nextToken();
    
    /**
     * 查看下一个标记（不消耗）
     * @return 标记
     */
    Token peekToken() const;
    
    /**
     * 保存当前词法分析器状态
     */
    void saveLexerState();
    
    /**
     * 恢复保存的词法分析器状态
     */
    void restoreLexerState();
    
private:
    Str m_source;        // 源代码
    size_t m_start = 0;          // 当前标记的起始位置
    size_t m_current = 0;        // 当前读取位置
    int m_line = 1;              // 当前行
    int m_column = 1;            // 当前列
    
    // 保存的状态
    struct LexerState {
        size_t start;
        size_t current;
        int line;
        int column;
    };
    
    LexerState m_savedState;     // 保存的状态
    
    // 辅助方法
    char peek() const;             // 查看当前字符
    char peekNext() const;         // 查看下一个字符
    char advance();                // 前进到下一个字符
    bool match(char expected);     // 如果当前字符匹配预期，则前进并返回true
    void skipWhitespace();         // 跳过空白字符
    void skipComment();            // 跳过注释
    Token makeToken(TokenType type); // 创建一个标记
    Token errorToken(const Str& message); // 创建一个错误标记
    
    // 各种标记的解析方法
    Token identifier();    // 解析标识符或关键字
    Token number();        // 解析数字字面量
    Token string();        // 解析字符串字面量
};

} // namespace LuaCore
