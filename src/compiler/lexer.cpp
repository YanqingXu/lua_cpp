#include "lexer.hpp"
#include "types.hpp"

#include <cctype>
#include <algorithm>
#include <sstream>

namespace Lua {

// 初始化关键字映射表
const HashMap<Str, TokenType> Lexer::s_keywords = {
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

Lexer::Lexer(const Str& source, const Str& sourceName)
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
    
    // 记录标记开始位置
    size_t startPos = m_position;
    int startLine = m_line;
    int startColumn = m_column;
    
    // 如果达到文件结尾，返回EOF标记
    if (m_position >= m_source.length()) {
        return makeToken(TokenType::Eof);
    }
    
    char c = advance();
    
    // 标识符和关键字
    if (isalpha(c) || c == '_') {
        return identifier();
    }
    
    // 数字
    if (isdigit(c)) {
        return number();
    }
    
    // 字符串
    if (c == '"' || c == '\'') {
        return string();
    }
    
    // 其他标记
    switch (c) {
        case '(':
            return makeToken(TokenType::LeftParen);
        case ')':
            return makeToken(TokenType::RightParen);
        case '{':
            return makeToken(TokenType::LeftBrace);
        case '}':
            return makeToken(TokenType::RightBrace);
        case '[':
            if (match('[')) {
                // 长字符串
                return string();
            }
            return makeToken(TokenType::LeftBracket);
        case ']':
            return makeToken(TokenType::RightBracket);
        case ';':
            return makeToken(TokenType::Semicolon);
        case ':':
            if (match(':')) {
                return makeToken(TokenType::DoubleColon);
            }
            return makeToken(TokenType::Colon);
        case ',':
            return makeToken(TokenType::Comma);
        case '.':
            if (match('.')) {
                if (match('.')) {
                    return makeToken(TokenType::Dots);
                }
                return makeToken(TokenType::Concat);
            }
            if (isdigit(peek())) {
                // 浮点数以.开头
                return number();
            }
            return makeToken(TokenType::Dot);
        case '+':
            return makeToken(TokenType::Plus);
        case '-':
            if (match('-')) {
                // 注释已在skipWhitespace中处理
                skipComment();
                return nextToken();
            }
            return makeToken(TokenType::Minus);
        case '*':
            return makeToken(TokenType::Star);
        case '/':
            return makeToken(TokenType::Slash);
        case '%':
            return makeToken(TokenType::Percent);
        case '^':
            return makeToken(TokenType::Caret);
        case '#':
            return makeToken(TokenType::Hash);
        case '=':
            if (match('=')) {
                return makeToken(TokenType::EqualEqual);
            }
            return makeToken(TokenType::Equal);
        case '~':
            if (match('=')) {
                return makeToken(TokenType::NotEqual);
            }
            return errorToken("Expected '=' after '~'");
        case '<':
            if (match('=')) {
                return makeToken(TokenType::LessEqual);
            }
            return makeToken(TokenType::Less);
        case '>':
            if (match('=')) {
                return makeToken(TokenType::GreaterEqual);
            }
            return makeToken(TokenType::Greater);
        default:
            return errorToken("Unexpected character");
    }
}

Token Lexer::peekToken() {
    if (m_hasNextToken) {
        return m_nextToken;
    }
    
    // 保存状态
    saveLexerState();
    m_nextToken = nextToken();
    m_hasNextToken = true;
    
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
    char c = '\0';
    if (m_position < m_source.length()) {
        c = m_source[m_position++];
    }
    
    // 更新行列号
    if (c == '\n') {
        m_line++;
        m_column = 0;
    } else {
        m_column++;
    }
    
    return c;
}

bool Lexer::match(char expected) {
    if (m_position >= m_source.length()) return false;
    if (m_source[m_position] != expected) return false;
    
    m_position++;
    m_column++;
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
    // 跳过"--"
    advance();
    advance();
    
    // 检查是否是长注释 --[[...]]
    if (peek() == '[') {
        advance();
        if (peek() == '[') {
            advance();
            // 长注释
            int depth = 1;
            while (m_position < m_source.length()) {
                if (peek() == ']' && peekNext() == ']') {
                    advance();
                    advance();
                    return;
                }
                advance();
            }
        }
    }
    
    // 单行注释，一直到行尾
    while (peek() != '\n' && peek() != '\0') {
        advance();
    }
}

Token Lexer::makeToken(TokenType type) {
    Token token;
    token.type = type;
    
    // 获取标记的实际文本
    size_t length = m_position - (m_position - (m_column == 0 ? 1 : m_column));
    token.lexeme = m_source.substr(m_position - length, length);
    
    token.line = m_line;
    token.column = m_column - length;
    
    return token;
}

Token Lexer::errorToken(const Str& message) {
    Token token;
    token.type = TokenType::Error;
    token.lexeme = "";
    token.line = m_line;
    token.column = m_column;
    token.stringValue = message;
    
    return token;
}

Token Lexer::identifier() {
    // 已经消费了第一个字符，继续消费字母数字和下划线
    while (isalnum(peek()) || peek() == '_') {
        advance();
    }
    
    // 获取标识符文本
    size_t length = m_column;
    size_t startPos = m_position - length;
    std::string text = m_source.substr(startPos, length);
    
    // 检查是否是关键字
    auto it = s_keywords.find(text);
    if (it != s_keywords.end()) {
        Token token = makeToken(it->second);
        return token;
    }
    
    // 普通标识符
    return makeToken(TokenType::Identifier);
}

Token Lexer::number() {
    // 确定起始位置
    size_t startPos = m_position - 1;
    
    // 处理整数部分
    while (isdigit(peek())) {
        advance();
    }
    
    // 处理小数部分
    if (peek() == '.' && isdigit(peekNext())) {
        advance(); // 消费 '.'
        
        while (isdigit(peek())) {
            advance();
        }
    }
    
    // 处理科学计数法 (e.g., 1e10, 2.5E-5)
    if (peek() == 'e' || peek() == 'E') {
        advance(); // 消费 'e' 或 'E'
        
        // 可选的符号
        if (peek() == '+' || peek() == '-') {
            advance();
        }
        
        // 指数必须是整数
        if (!isdigit(peek())) {
            return errorToken("Invalid number: expected digit after exponent");
        }
        
        while (isdigit(peek())) {
            advance();
        }
    }
    
    // 提取数字文本
    std::string numberStr = m_source.substr(startPos, m_position - startPos);
    
    // 转换为数值
    double value = std::stod(numberStr);
    
    // 创建标记
    Token token = makeToken(TokenType::Number);
    token.numberValue = value;
    
    return token;
}

Token Lexer::string() {
    char delimiter = m_source[m_position - 1]; // 获取字符串的开始分隔符
    std::ostringstream contentStream;
    
    if (delimiter == '[') {
        // 处理长字符串 [[...]] 或 [=[...]=]
        int equalsCount = 0;
        
        // 计算等号数量
        while (peek() == '=') {
            equalsCount++;
            advance();
        }
        
        // 确保开始正确
        if (peek() != '[') {
            return errorToken("Expected '[' after '['");
        }
        advance(); // 消费 '['
        
        // 读取内容直到匹配的结束分隔符
        while (m_position < m_source.length()) {
            if (peek() == ']') {
                advance(); // 消费 ']'
                
                // 检查等号数量
                bool matches = true;
                for (int i = 0; i < equalsCount; i++) {
                    if (peek() != '=') {
                        matches = false;
                        break;
                    }
                    advance();
                }
                
                // 检查第二个 ']'
                if (matches && peek() == ']') {
                    advance(); // 消费 ']'
                    
                    // 创建字符串标记
                    Token token = makeToken(TokenType::String);
                    token.stringValue = contentStream.str();
                    return token;
                }
                
                // 如果不匹配，则将已读取的字符加入内容
                contentStream << ']';
                for (int i = 0; i < equalsCount && !matches; i++) {
                    contentStream << '=';
                }
                continue;
            }
            
            // 读取下一个字符
            contentStream << advance();
        }
        
        // 如果到达这里，表示长字符串未终止
        return errorToken("Unterminated long string");
    }
    else {
        // 处理普通字符串 '...' 或 "..."
        while (peek() != delimiter && peek() != '\0') {
            if (peek() == '\n') {
                return errorToken("Unterminated string");
            }
            
            if (peek() == '\\') {
                advance(); // 消费 '\'
                
                // 处理转义序列
                switch (peek()) {
                    case 'a':  contentStream << '\a'; break;
                    case 'b':  contentStream << '\b'; break;
                    case 'f':  contentStream << '\f'; break;
                    case 'n':  contentStream << '\n'; break;
                    case 'r':  contentStream << '\r'; break;
                    case 't':  contentStream << '\t'; break;
                    case 'v':  contentStream << '\v'; break;
                    case '\\': contentStream << '\\'; break;
                    case '\'': contentStream << '\''; break;
                    case '"':  contentStream << '"';  break;
                    case '\n': contentStream << '\n'; break; // 续行
                    case 'x': {
                        // 十六进制转义 \xXX
                        advance(); // 消费 'x'
                        
                        char hex[3] = {0};
                        if (isxdigit(peek()) && isxdigit(peekNext())) {
                            hex[0] = advance();
                            hex[1] = advance();
                            
                            // 转换十六进制为字符
                            int value;
                            std::sscanf(hex, "%x", &value);
                            contentStream << static_cast<char>(value);
                        }
                        else {
                            return errorToken("Invalid hex escape sequence");
                        }
                        break;
                    }
                    default:
                        if (isdigit(peek())) {
                            // 十进制转义 \d \dd \ddd
                            char dec[4] = {0};
                            int i = 0;
                            
                            while (isdigit(peek()) && i < 3) {
                                dec[i++] = advance();
                            }
                            
                            // 转换十进制为字符
                            int value = std::stoi(dec);
                            if (value > 255) {
                                return errorToken("Decimal escape too large");
                            }
                            
                            contentStream << static_cast<char>(value);
                        }
                        else {
                            contentStream << peek(); // 未知转义，直接输出
                        }
                        break;
                }
                
                advance(); // 消费转义字符
                continue;
            }
            
            // 普通字符
            contentStream << advance();
        }
        
        // 确保字符串正确终止
        if (peek() == '\0') {
            return errorToken("Unterminated string");
        }
        
        advance(); // 消费结束分隔符
        
        // 创建字符串标记
        Token token = makeToken(TokenType::String);
        token.stringValue = contentStream.str();
        return token;
    }
}

void Lexer::saveLexerState() {
    m_savedState.position = m_position;
    m_savedState.line = m_line;
    m_savedState.column = m_column;
}

void Lexer::restoreLexerState() {
    m_position = m_savedState.position;
    m_line = m_savedState.line;
    m_column = m_savedState.column;
}
} // namespace Lua
