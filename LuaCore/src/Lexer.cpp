#include "LuaCore/Lexer.h"
#include <cctype>
#include <algorithm>

namespace LuaCore {

// 初始化关键字映射表
const std::unordered_map<std::string, TokenType> Lexer::s_keywords = {
    {"and", TokenType::And},
    {"break", TokenType::Break},
    {"do", TokenType::Do},
    {"else", TokenType::Else},
    {"elseif", TokenType::Elseif},
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

Lexer::Lexer(const std::string& source, const std::string& sourceName)
    : m_source(source), m_sourceName(sourceName) {
}

Token Lexer::nextToken() {
    // 如果已经预读了下一个标记，则返回它
    if (m_hasNextToken) {
        m_hasNextToken = false;
        return m_nextToken;
    }
    
    // 跳过空白字符和注释
    skipWhitespace();
    
    // 记录标记开始位置的行列号
    int startLine = m_line;
    int startColumn = m_column;
    
    // 如果达到文件结尾，返回EOF标记
    if (m_position >= m_source.length()) {
        Token token;
        token.type = TokenType::EndOfFile;
        token.lexeme = "";
        token.line = startLine;
        token.column = startColumn;
        return token;
    }
    
    // 获取当前字符
    char c = peek();
    
    // 标识符和关键字
    if (std::isalpha(c) || c == '_') {
        return identifier();
    }
    
    // 数字
    if (std::isdigit(c)) {
        return number();
    }
    
    // 根据当前字符确定标记类型
    switch (c) {
        // 单字符标记
        case '(': advance(); return makeToken(TokenType::LeftParen);
        case ')': advance(); return makeToken(TokenType::RightParen);
        case '{': advance(); return makeToken(TokenType::LeftBrace);
        case '}': advance(); return makeToken(TokenType::RightBrace);
        case '[': advance(); return makeToken(TokenType::LeftBracket);
        case ']': advance(); return makeToken(TokenType::RightBracket);
        case ',': advance(); return makeToken(TokenType::Comma);
        case ';': advance(); return makeToken(TokenType::Semicolon);
        case '%': advance(); return makeToken(TokenType::Percent);
        case '^': advance(); return makeToken(TokenType::Caret);
        case '#': advance(); return makeToken(TokenType::Hash);
        
        // 可能是多字符标记
        case '+': advance(); return makeToken(TokenType::Plus);
        case '*': advance(); return makeToken(TokenType::Star);
        
        case '-':
            advance();
            // 检查是否是注释
            if (peek() == '-') {
                skipComment();
                return nextToken();
            }
            return makeToken(TokenType::Minus);
            
        case '/': advance(); return makeToken(TokenType::Slash);
        
        // 点操作符可能是'.', '..' 或数字
        case '.':
            advance();
            if (peek() == '.') {
                advance();
                if (peek() == '.') {
                    advance();
                    return makeToken(TokenType::Dot); // 变长参数'...'
                }
                return makeToken(TokenType::Concat); // 字符串连接'..'
            }
            if (std::isdigit(peek())) {
                // 如果点后面跟着数字，那么这是一个以小数点开始的数字
                m_position--; // 回退一个字符，这样number()可以正确处理
                m_column--;
                return number();
            }
            return makeToken(TokenType::Dot);
        
        // 比较和赋值运算符
        case '=':
            advance();
            if (match('=')) return makeToken(TokenType::Equal);
            return makeToken(TokenType::Assign);
            
        case '~':
            advance();
            if (match('=')) return makeToken(TokenType::NotEqual);
            return errorToken("Unexpected character after '~'");
            
        case '<':
            advance();
            if (match('=')) return makeToken(TokenType::LessEqual);
            return makeToken(TokenType::LessThan);
            
        case '>':
            advance();
            if (match('=')) return makeToken(TokenType::GreaterEqual);
            return makeToken(TokenType::GreaterThan);
            
        // 冒号和双冒号
        case ':':
            advance();
            if (match(':')) return makeToken(TokenType::DoubleColon);
            return makeToken(TokenType::Colon);
            
        // 字符串
        case '"':
        case '\'':
            return string();
    }
    
    // 如果到达这里，说明遇到了未知字符
    advance();
    return errorToken("Unexpected character");
}

Token Lexer::peekToken() {
    if (!m_hasNextToken) {
        m_nextToken = nextToken();
        m_hasNextToken = true;
    }
    return m_nextToken;
}

char Lexer::peek() const {
    if (m_position >= m_source.length()) return '\0';
    return m_source[m_position];
}

char Lexer::peekNext() const {
    if (m_position + 1 >= m_source.length()) return '\0';
    return m_source[m_position + 1];
}

char Lexer::advance() {
    char c = peek();
    m_position++;
    
    if (c == '\n') {
        m_line++;
        m_column = 1;
    } else {
        m_column++;
    }
    
    return c;
}

bool Lexer::match(char expected) {
    if (peek() != expected) return false;
    advance();
    return true;
}

void Lexer::skipWhitespace() {
    while (true) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\t':
            case '\r':
                advance();
                break;
                
            case '\n':
                advance();
                break;
                
            case '-':
                // 检查是否是注释
                if (peekNext() == '-') {
                    skipComment();
                } else {
                    return;
                }
                break;
                
            default:
                return;
        }
    }
}

void Lexer::skipComment() {
    // 消耗两个'-'字符
    advance();
    advance();
    
    // 检查是否是长注释（--[[...]]）
    if (peek() == '[' && peekNext() == '[') {
        advance(); // 消耗第一个'['
        advance(); // 消耗第二个'['
        
        // 查找闭合的']]'
        while (m_position < m_source.length()) {
            if (peek() == ']' && peekNext() == ']') {
                advance(); // 消耗第一个']'
                advance(); // 消耗第二个']'
                return;
            }
            advance();
        }
        
        // 如果到达这里，说明没有找到闭合的']]'
        return;
    }
    
    // 普通注释，持续到行尾
    while (peek() != '\n' && peek() != '\0') {
        advance();
    }
}

Token Lexer::makeToken(TokenType type) {
    Token token;
    token.type = type;
    token.lexeme = m_source.substr(m_position - token.lexeme.length(), token.lexeme.length());
    token.line = m_line;
    token.column = m_column - token.lexeme.length();
    return token;
}

Token Lexer::errorToken(const std::string& message) {
    Token token;
    token.type = TokenType::Error;
    token.lexeme = message;
    token.line = m_line;
    token.column = m_column;
    return token;
}

Token Lexer::identifier() {
    // 标识符的起始位置
    size_t start = m_position;
    int startColumn = m_column;
    
    // 消耗第一个字符（已知是字母或下划线）
    advance();
    
    // 消耗剩余的字母、数字或下划线
    while (std::isalnum(peek()) || peek() == '_') {
        advance();
    }
    
    // 提取标识符文本
    std::string text = m_source.substr(start, m_position - start);
    
    // 检查是否是关键字
    TokenType type = TokenType::Identifier;
    auto it = s_keywords.find(text);
    if (it != s_keywords.end()) {
        type = it->second;
    }
    
    // 创建标记
    Token token;
    token.type = type;
    token.lexeme = text;
    token.line = m_line;
    token.column = startColumn;
    return token;
}

Token Lexer::number() {
    // 数字的起始位置
    size_t start = m_position;
    int startColumn = m_column;
    
    // 处理整数部分
    while (std::isdigit(peek())) {
        advance();
    }
    
    // 处理小数部分
    if (peek() == '.' && std::isdigit(peekNext())) {
        // 消耗小数点
        advance();
        
        // 消耗小数部分的数字
        while (std::isdigit(peek())) {
            advance();
        }
    }
    
    // 处理指数部分
    if (peek() == 'e' || peek() == 'E') {
        // 消耗'e'或'E'
        advance();
        
        // 消耗可选的符号
        if (peek() == '+' || peek() == '-') {
            advance();
        }
        
        // 至少需要一个数字
        if (!std::isdigit(peek())) {
            return errorToken("Invalid number format: expected digit after exponent");
        }
        
        // 消耗指数部分的数字
        while (std::isdigit(peek())) {
            advance();
        }
    }
    
    // 提取数字文本并转换为double
    std::string text = m_source.substr(start, m_position - start);
    double value = std::stod(text);
    
    // 创建标记
    Token token;
    token.type = TokenType::Number;
    token.lexeme = text;
    token.numberValue = value;
    token.line = m_line;
    token.column = startColumn;
    return token;
}

Token Lexer::string() {
    // 记录字符串的起始引号
    char quoteChar = peek();
    int startLine = m_line;
    int startColumn = m_column;
    
    // 消耗起始引号
    advance();
    
    // 用于存储处理后的字符串值
    std::string value;
    
    // 消耗字符串内容直到遇到结束引号
    while (peek() != quoteChar && peek() != '\0') {
        // 处理转义序列
        if (peek() == '\\') {
            advance(); // 消耗反斜杠
            
            // 处理各种转义序列
            switch (peek()) {
                case 'a': advance(); value += '\a'; break;
                case 'b': advance(); value += '\b'; break;
                case 'f': advance(); value += '\f'; break;
                case 'n': advance(); value += '\n'; break;
                case 'r': advance(); value += '\r'; break;
                case 't': advance(); value += '\t'; break;
                case 'v': advance(); value += '\v'; break;
                case '\\': advance(); value += '\\'; break;
                case '\'': advance(); value += '\''; break;
                case '"': advance(); value += '"'; break;
                    
                // 十进制转义序列 \ddd
                case '0': case '1': case '2': case '3': case '4':
                case '5': case '6': case '7': case '8': case '9': {
                    // 收集最多3个数字
                    std::string digits;
                    for (int i = 0; i < 3 && std::isdigit(peek()); i++) {
                        digits += advance();
                    }
                    // 转换为数字并添加对应的字符
                    int code = std::stoi(digits);
                    if (code <= 255) {
                        value += static_cast<char>(code);
                    } else {
                        // 错误：无效的字符代码
                        return errorToken("Invalid escape sequence: decimal value too large");
                    }
                    break;
                }
                    
                // 其他未处理的转义序列
                default:
                    // Lua允许\z跳过后面的空白字符
                    if (peek() == 'z') {
                        advance(); // 消耗'z'
                        // 跳过后续空白字符
                        while (std::isspace(peek())) {
                            advance();
                        }
                    } else {
                        // 其他字符直接保留
                        value += advance();
                    }
                    break;
            }
        } else {
            // 普通字符，直接添加
            value += advance();
        }
        
        // 检查是否达到行尾（未闭合的字符串）
        if (peek() == '\n' || peek() == '\0') {
            return errorToken("Unterminated string");
        }
    }
    
    // 如果到达这里且下一个字符不是结束引号，说明提前遇到了文件结尾
    if (peek() == '\0') {
        return errorToken("Unterminated string");
    }
    
    // 消耗结束引号
    advance();
    
    // 创建标记
    Token token;
    token.type = TokenType::String;
    token.lexeme = m_source.substr(m_position - token.lexeme.length() - 2, token.lexeme.length() + 2);
    token.stringValue = value;
    token.line = startLine;
    token.column = startColumn;
    return token;
}

Token Lexer::peek() const {
    // 保存当前状态
    size_t oldStart = m_start;
    size_t oldCurrent = m_current;
    int oldLine = m_line;
    int oldColumn = m_column;
    
    // 创建临时Lexer对象复制当前状态
    Lexer tempLexer = *this;
    
    // 获取下一个token
    Token result = tempLexer.nextToken();
    
    // 恢复原始状态 (因为是const方法，我们创建了临时对象而非修改当前对象)
    return result;
}

void Lexer::saveLexerState() {
    m_savedState.start = m_start;
    m_savedState.current = m_current;
    m_savedState.line = m_line;
    m_savedState.column = m_column;
}

void Lexer::restoreLexerState() {
    m_start = m_savedState.start;
    m_current = m_savedState.current;
    m_line = m_savedState.line;
    m_column = m_savedState.column;
}

} // namespace LuaCore
