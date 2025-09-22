/**
 * @file lexer.cpp
 * @brief Lua词法分析器实现
 * @description 基于Lua 5.1.5源码的词法分析器核心功能实现
 * @author Lua C++ Project
 * @date 2025-09-21
 */

#include "lexer.h"
#include "../core/lua_errors.h"
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <fstream>

namespace lua_cpp {

/* ========================================================================== */
/* 辅助结构体 */
/* ========================================================================== */

struct TokenPosition {
    int line;
    int column;
    
    TokenPosition(int l, int c) : line(l), column(c) {}
};

/* ========================================================================== */
/* StringInputStream 实现 */
/* ========================================================================== */

StringInputStream::StringInputStream(const std::string& source, std::string_view source_name)
    : source_(source), source_name_(source_name), position_(0) {
}

StringInputStream::StringInputStream(std::string&& source, std::string_view source_name)
    : source_(std::move(source)), source_name_(source_name), position_(0) {
}

int StringInputStream::NextChar() {
    if (position_ >= source_.size()) {
        return -1; // EOZ
    }
    return static_cast<unsigned char>(source_[position_++]);
}

Size StringInputStream::GetPosition() const {
    return position_;
}

bool StringInputStream::IsAtEnd() const {
    return position_ >= source_.size();
}

std::string_view StringInputStream::GetSourceName() const {
    return source_name_;
}

/* ========================================================================== */
/* FileInputStream 实现 */
/* ========================================================================== */

class FileInputStream::Impl {
public:
    std::ifstream file_;
    std::string filename_;
    Size position_;
    
    Impl(const std::string& filename) 
        : file_(filename, std::ios::binary), filename_(filename), position_(0) {
        if (!file_.is_open()) {
            throw std::runtime_error("Cannot open file: " + filename);
        }
    }
};

FileInputStream::FileInputStream(const std::string& filename)
    : impl_(std::make_unique<Impl>(filename)) {
}

FileInputStream::~FileInputStream() = default;

int FileInputStream::NextChar() {
    int c = impl_->file_.get();
    if (c != EOF) {
        ++impl_->position_;
    }
    return (c == EOF) ? -1 : c;
}

Size FileInputStream::GetPosition() const {
    return impl_->position_;
}

bool FileInputStream::IsAtEnd() const {
    return impl_->file_.eof();
}

std::string_view FileInputStream::GetSourceName() const {
    return impl_->filename_;
}

/* ========================================================================== */
/* TokenBuffer 实现 */
/* ========================================================================== */

TokenBuffer::TokenBuffer() : TokenBuffer(512) {
}

TokenBuffer::TokenBuffer(Size initial_capacity) : size_(0) {
    buffer_.reserve(initial_capacity);
}

void TokenBuffer::Clear() {
    buffer_.clear();
    size_ = 0;
}

void TokenBuffer::AppendChar(char ch) {
    buffer_.push_back(ch);
    ++size_;
}

void TokenBuffer::AppendString(std::string_view str) {
    buffer_.insert(buffer_.end(), str.begin(), str.end());
    size_ += str.size();
}

std::string_view TokenBuffer::GetContent() const {
    return std::string_view(buffer_.data(), size_);
}

std::string TokenBuffer::ToString() const {
    return std::string(buffer_.data(), size_);
}

Size TokenBuffer::GetSize() const {
    return size_;
}

Size TokenBuffer::GetCapacity() const {
    return buffer_.capacity();
}

bool TokenBuffer::IsEmpty() const {
    return size_ == 0;
}

void TokenBuffer::Reserve(Size capacity) {
    buffer_.reserve(capacity);
}

/* ========================================================================== */
/* LexicalError 实现 */
/* ========================================================================== */

LexicalError::LexicalError(const std::string& message, const TokenPosition& position)
    : LuaException(ErrorCode::SyntaxError, message), position_(position) {
}

LexicalError::LexicalError(const std::string& message, Size line, Size column, std::string_view source)
    : LuaException(ErrorCode::SyntaxError, message), position_(line, column, std::string(source)) {
}

/* ========================================================================== */
/* Lexer 实现 */
/* ========================================================================== */

Lexer::Lexer(std::unique_ptr<InputStream> input, const LexerConfig& config)
    : input_(std::move(input)), config_(config), current_char_(0), 
      current_line_(1), current_column_(1), last_line_(1),
      has_lookahead_(false), token_count_(0) {
    
    if (!input_) {
        throw std::invalid_argument("Input stream cannot be null");
    }
    
    // 读取第一个字符
    NextChar();
    
    // 初始化当前Token为EndOfSource
    current_token_ = Token::CreateEndOfSource(TokenPosition(1, 1));
}

Lexer::Lexer(const std::string& source, std::string_view source_name, const LexerConfig& config)
    : Lexer(std::make_unique<StringInputStream>(source, source_name), config) {
}

Token Lexer::NextToken() {
    if (has_lookahead_) {
        has_lookahead_ = false;
        current_token_ = lookahead_token_;
        return current_token_;
    }
    
    current_token_ = ScanToken();
    ++token_count_;
    return current_token_;
}

Token Lexer::PeekToken() {
    if (!has_lookahead_) {
        // 实现前瞻需要保存和恢复状态
        // 简化实现：重新扫描（在完整实现中需要状态保存/恢复）
        lookahead_token_ = ScanToken();
        has_lookahead_ = true;
    }
    return lookahead_token_;
}

bool Lexer::Check(TokenType expected_type) {
    return current_token_.GetType() == expected_type;
}

bool Lexer::Match(TokenType expected_type) {
    if (Check(expected_type)) {
        NextToken();
        return true;
    }
    return false;
}

Token Lexer::Expect(TokenType expected_type) {
    if (!Check(expected_type)) {
        std::stringstream ss;
        ss << "Expected " << static_cast<int>(expected_type) 
           << " but got " << static_cast<int>(current_token_.GetType());
        CreateError(ss.str());
    }
    return NextToken();
}

bool Lexer::IsAtEnd() const {
    return current_token_.GetType() == TokenType::EndOfSource;
}

Size Lexer::GetCurrentOffset() const {
    return input_->GetPosition();
}

std::string_view Lexer::GetSourceName() const {
    return input_->GetSourceName();
}

TokenPosition Lexer::GetCurrentPosition() const {
    return TokenPosition(current_line_, current_column_);
}

Size Lexer::GetTokenCount() const {
    return token_count_;
}

void Lexer::ResetStatistics() {
    token_count_ = 0;
}

/* ========================================================================== */
/* 私有方法实现 - 字符处理 */
/* ========================================================================== */

void Lexer::NextChar() {
    if (current_char_ != -1) {
        if (current_char_ == '\n') {
            last_line_ = current_line_;
            ++current_line_;
            current_column_ = 1;
        } else {
            ++current_column_;
        }
        
        current_char_ = input_->NextChar();
    }
}

void Lexer::SkipWhitespace() {
    while (IsWhitespace(current_char_)) {
        NextChar();
    }
}

void Lexer::SkipLineComment() {
    // 跳过到行尾
    while (current_char_ != '\n' && current_char_ != -1) {
        NextChar();
    }
}

void Lexer::SkipBlockComment(Size sep_length) {
    // 查找对应的结束标记 ]===]
    Size count = 0;
    while (current_char_ != -1) {
        if (current_char_ == ']') {
            NextChar();
            Size found_sep = 0;
            while (current_char_ == '=' && found_sep < sep_length) {
                NextChar();
                ++found_sep;
            }
            if (found_sep == sep_length && current_char_ == ']') {
                NextChar(); // 跳过结束的 ]
                return;
            }
        } else {
            NextChar();
        }
    }
    // 如果到这里说明注释没有正确结束
    CreateError("Unfinished long comment");
}

int Lexer::CheckLongSeparator() {
    // 检查 [===[ 格式
    if (current_char_ != '[') {
        return -1;
    }
    
    Size saved_pos = input_->GetPosition();
    Size saved_line = current_line_;
    Size saved_column = current_column_;
    int saved_char = current_char_;
    
    NextChar(); // 跳过第一个 '['
    Size sep_count = 0;
    
    // 计算等号数量
    while (current_char_ == '=') {
        NextChar();
        ++sep_count;
    }
    
    if (current_char_ == '[') {
        NextChar(); // 跳过第二个 '['
        return static_cast<int>(sep_count);
    }
    
    // 恢复状态
    // 注意：这是简化实现，实际中需要支持输入流的回退
    current_char_ = saved_char;
    current_line_ = saved_line;
    current_column_ = saved_column;
    return -1;
}

/* ========================================================================== */
/* 私有方法实现 - Token识别 */
/* ========================================================================== */

Token Lexer::ScanToken() {
    // 跳过空白符和注释
    while (true) {
        SkipWhitespace();
        
        // 检查注释
        if (current_char_ == '-' && input_->NextChar() == '-') {
            NextChar(); // 跳过第一个 '-'
            NextChar(); // 跳过第二个 '-'
            
            // 检查长注释
            int sep_length = CheckLongSeparator();
            if (sep_length >= 0) {
                SkipBlockComment(sep_length);
            } else {
                SkipLineComment();
            }
            continue;
        }
        break;
    }
    
    // 保存Token开始位置
    TokenPosition start_pos(current_line_, current_column_);
    
    // 检查是否到达末尾
    if (current_char_ == -1) {
        return Token::CreateEndOfSource(start_pos);
    }
    
    int c = current_char_;
    
    // 数字
    if (IsDigit(c) || (c == '.' && IsDigit(input_->NextChar()))) {
        return ReadNumber();
    }
    
    // 字符串
    if (c == '"' || c == '\'') {
        return ReadString(static_cast<char>(c));
    }
    
    // 长字符串 [[ 或 [====[
    if (c == '[') {
        int sep_length = CheckLongSeparator();
        if (sep_length >= 0) {
            return ReadLongString(sep_length);
        }
    }
    
    // 标识符和关键字
    if (IsAlpha(c)) {
        return ReadName();
    }
    
    // 操作符和分隔符
    NextChar(); // 消费字符
    
    switch (c) {
        case '+': return Token::CreateOperator(TokenType::Plus, start_pos.line, start_pos.column);
        case '-': return Token::CreateOperator(TokenType::Minus, start_pos.line, start_pos.column);
        case '*': return Token::CreateOperator(TokenType::Multiply, start_pos.line, start_pos.column);
        case '/': return Token::CreateOperator(TokenType::Divide, start_pos.line, start_pos.column);
        case '%': return Token::CreateOperator(TokenType::Modulo, start_pos.line, start_pos.column);
        case '^': return Token::CreateOperator(TokenType::Power, start_pos.line, start_pos.column);
        case '#': return Token::CreateOperator(TokenType::Length, start_pos.line, start_pos.column);
        
        case '(': return Token::CreateDelimiter(TokenType::LeftParen, start_pos.line, start_pos.column);
        case ')': return Token::CreateDelimiter(TokenType::RightParen, start_pos.line, start_pos.column);
        case '{': return Token::CreateDelimiter(TokenType::LeftBrace, start_pos.line, start_pos.column);
        case '}': return Token::CreateDelimiter(TokenType::RightBrace, start_pos.line, start_pos.column);
        case '[': return Token::CreateDelimiter(TokenType::LeftBracket, start_pos.line, start_pos.column);
        case ']': return Token::CreateDelimiter(TokenType::RightBracket, start_pos.line, start_pos.column);
        case ';': return Token::CreateDelimiter(TokenType::Semicolon, start_pos.line, start_pos.column);
        case ',': return Token::CreateDelimiter(TokenType::Comma, start_pos.line, start_pos.column);
        
        // 多字符操作符
        case '<':
            if (current_char_ == '=') {
                NextChar();
                return Token::CreateOperator(TokenType::LessEqual, start_pos.line, start_pos.column);
            }
            return Token::CreateOperator(TokenType::Less, start_pos.line, start_pos.column);
            
        case '>':
            if (current_char_ == '=') {
                NextChar();
                return Token::CreateOperator(TokenType::GreaterEqual, start_pos.line, start_pos.column);
            }
            return Token::CreateOperator(TokenType::Greater, start_pos.line, start_pos.column);
            
        case '=':
            if (current_char_ == '=') {
                NextChar();
                return Token::CreateOperator(TokenType::Equal, start_pos.line, start_pos.column);
            }
            return Token::CreateOperator(TokenType::Assign, start_pos.line, start_pos.column);
            
        case '~':
            if (current_char_ == '=') {
                NextChar();
                return Token::CreateOperator(TokenType::NotEqual, start_pos.line, start_pos.column);
            }
            CreateError("Invalid character '~'");
            break;
            
        case '.':
            if (current_char_ == '.') {
                NextChar();
                if (current_char_ == '.') {
                    NextChar();
                    return Token::CreateOperator(TokenType::Concat, start_pos.line, start_pos.column);
                }
                return Token::CreateOperator(TokenType::Concat, start_pos.line, start_pos.column);
            }
            return Token::CreateOperator(TokenType::Dot, start_pos.line, start_pos.column);
            
        case ':':
            return Token::CreateOperator(TokenType::Colon, start_pos.line, start_pos.column);
            
        default:
            std::stringstream ss;
            ss << "Unexpected character '" << static_cast<char>(c) << "' (ASCII " << c << ")";
            CreateError(ss.str());
    }
    
    // 不应该到达这里
    return Token::CreateEndOfSource(start_pos);
}

Token Lexer::ReadNumber() {
    TokenPosition start_pos(current_line_, current_column_);
    buffer_.Clear();
    
    bool has_dot = false;
    bool has_exp = false;
    
    // 处理小数点开始的数字 .123
    if (current_char_ == '.') {
        has_dot = true;
        buffer_.AppendChar('.');
        NextChar();
    }
    
    // 读取数字部分
    while (IsDigit(current_char_)) {
        buffer_.AppendChar(static_cast<char>(current_char_));
        NextChar();
    }
    
    // 小数点
    if (!has_dot && current_char_ == '.') {
        // 检查不是 .. 操作符
        if (input_->NextChar() != '.') {
            has_dot = true;
            buffer_.AppendChar('.');
            NextChar();
            
            // 小数部分
            while (IsDigit(current_char_)) {
                buffer_.AppendChar(static_cast<char>(current_char_));
                NextChar();
            }
        }
    }
    
    // 科学计数法
    if (current_char_ == 'e' || current_char_ == 'E') {
        has_exp = true;
        buffer_.AppendChar(static_cast<char>(current_char_));
        NextChar();
        
        // 可选的符号
        if (current_char_ == '+' || current_char_ == '-') {
            buffer_.AppendChar(static_cast<char>(current_char_));
            NextChar();
        }
        
        // 指数部分必须有数字
        if (!IsDigit(current_char_)) {
            CreateError("Malformed number");
        }
        
        while (IsDigit(current_char_)) {
            buffer_.AppendChar(static_cast<char>(current_char_));
            NextChar();
        }
    }
    
    std::string number_str = buffer_.ToString();
    return Token::CreateNumber(number_str, start_pos.line, start_pos.column);
}

Token Lexer::ReadString(char quote) {
    TokenPosition start_pos(current_line_, current_column_);
    buffer_.Clear();
    
    NextChar(); // 跳过开始引号
    
    while (current_char_ != quote && current_char_ != -1) {
        if (current_char_ == '\\') {
            NextChar();
            if (current_char_ == -1) {
                CreateError("Unfinished string");
            }
            
            // 转义字符处理
            switch (current_char_) {
                case 'n': buffer_.AppendChar('\n'); break;
                case 't': buffer_.AppendChar('\t'); break;
                case 'r': buffer_.AppendChar('\r'); break;
                case 'b': buffer_.AppendChar('\b'); break;
                case 'f': buffer_.AppendChar('\f'); break;
                case 'v': buffer_.AppendChar('\v'); break;
                case 'a': buffer_.AppendChar('\a'); break;
                case '\\': buffer_.AppendChar('\\'); break;
                case '"': buffer_.AppendChar('"'); break;
                case '\'': buffer_.AppendChar('\''); break;
                case '\n': 
                    current_line_++;
                    current_column_ = 1;
                    buffer_.AppendChar('\n'); 
                    break;
                case 'z': // Lua 5.1的z转义：跳过后续空白符
                    NextChar();
                    SkipWhitespace();
                    continue;
                default:
                    if (IsDigit(current_char_)) {
                        // 数字转义 \ddd
                        int value = 0;
                        for (int i = 0; i < 3 && IsDigit(current_char_); i++) {
                            value = value * 10 + (current_char_ - '0');
                            NextChar();
                        }
                        if (value > 255) {
                            CreateError("Escape sequence too large");
                        }
                        buffer_.AppendChar(static_cast<char>(value));
                        continue;
                    } else {
                        // 未知转义字符
                        buffer_.AppendChar(static_cast<char>(current_char_));
                    }
                    break;
            }
            NextChar();
        } else if (current_char_ == '\n') {
            CreateError("Unfinished string");
        } else {
            buffer_.AppendChar(static_cast<char>(current_char_));
            NextChar();
        }
    }
    
    if (current_char_ != quote) {
        CreateError("Unfinished string");
    }
    
    NextChar(); // 跳过结束引号
    
    std::string string_value = buffer_.ToString();
    return Token::CreateString(string_value, start_pos.line, start_pos.column);
}

Token Lexer::ReadLongString(int sep_length) {
    TokenPosition start_pos(current_line_, current_column_);
    buffer_.Clear();
    
    // 跳过开始分隔符 [=*[
    for (int i = 0; i <= sep_length + 1; i++) {
        NextChar();
    }
    
    // 如果第一个字符是换行符，跳过它
    if (current_char_ == '\n') {
        NextChar();
    }
    
    while (current_char_ != -1) {
        if (current_char_ == ']') {
            // 检查是否是结束分隔符
            int saved_char = current_char_;
            int saved_line = current_line_;
            int saved_column = current_column_;
            
            NextChar();
            bool is_end = true;
            
            // 检查=的数量
            for (int i = 0; i < sep_length; i++) {
                if (current_char_ != '=') {
                    is_end = false;
                    break;
                }
                NextChar();
            }
            
            if (is_end && current_char_ == ']') {
                NextChar(); // 跳过最后的]
                std::string string_value = buffer_.ToString();
                return Token::CreateString(string_value, start_pos.line, start_pos.column);
            }
            
            // 不是结束分隔符，恢复并添加到buffer
            current_char_ = saved_char;
            current_line_ = saved_line;
            current_column_ = saved_column;
            buffer_.AppendChar(static_cast<char>(current_char_));
            NextChar();
        } else {
            if (current_char_ == '\n') {
                current_line_++;
                current_column_ = 0;
            }
            buffer_.AppendChar(static_cast<char>(current_char_));
            NextChar();
        }
    }
    
    CreateError("Unfinished long string");
    return Token::CreateEndOfSource(start_pos); // 不会到达
}

Token Lexer::ReadName() {
    TokenPosition start_pos(current_line_, current_column_);
    buffer_.Clear();
    
    // 读取标识符字符
    while (IsAlnum(current_char_) || current_char_ == '_') {
        buffer_.AppendChar(static_cast<char>(current_char_));
        NextChar();
    }
    
    std::string name = buffer_.ToString();
    
    // 检查是否是关键字
    if (name == "and") return Token::CreateKeyword(TokenType::And, start_pos.line, start_pos.column);
    if (name == "break") return Token::CreateKeyword(TokenType::Break, start_pos.line, start_pos.column);
    if (name == "do") return Token::CreateKeyword(TokenType::Do, start_pos.line, start_pos.column);
    if (name == "else") return Token::CreateKeyword(TokenType::Else, start_pos.line, start_pos.column);
    if (name == "elseif") return Token::CreateKeyword(TokenType::Elseif, start_pos.line, start_pos.column);
    if (name == "end") return Token::CreateKeyword(TokenType::End, start_pos.line, start_pos.column);
    if (name == "false") return Token::CreateKeyword(TokenType::False, start_pos.line, start_pos.column);
    if (name == "for") return Token::CreateKeyword(TokenType::For, start_pos.line, start_pos.column);
    if (name == "function") return Token::CreateKeyword(TokenType::Function, start_pos.line, start_pos.column);
    if (name == "goto") return Token::CreateKeyword(TokenType::Goto, start_pos.line, start_pos.column);
    if (name == "if") return Token::CreateKeyword(TokenType::If, start_pos.line, start_pos.column);
    if (name == "in") return Token::CreateKeyword(TokenType::In, start_pos.line, start_pos.column);
    if (name == "local") return Token::CreateKeyword(TokenType::Local, start_pos.line, start_pos.column);
    if (name == "nil") return Token::CreateKeyword(TokenType::Nil, start_pos.line, start_pos.column);
    if (name == "not") return Token::CreateKeyword(TokenType::Not, start_pos.line, start_pos.column);
    if (name == "or") return Token::CreateKeyword(TokenType::Or, start_pos.line, start_pos.column);
    if (name == "repeat") return Token::CreateKeyword(TokenType::Repeat, start_pos.line, start_pos.column);
    if (name == "return") return Token::CreateKeyword(TokenType::Return, start_pos.line, start_pos.column);
    if (name == "then") return Token::CreateKeyword(TokenType::Then, start_pos.line, start_pos.column);
    if (name == "true") return Token::CreateKeyword(TokenType::True, start_pos.line, start_pos.column);
    if (name == "until") return Token::CreateKeyword(TokenType::Until, start_pos.line, start_pos.column);
    if (name == "while") return Token::CreateKeyword(TokenType::While, start_pos.line, start_pos.column);
    
    // 不是关键字，返回标识符
    return Token::CreateIdentifier(name, start_pos.line, start_pos.column);
}

void Lexer::CreateError(const std::string& message) {
    std::stringstream ss;
    ss << "Lexical error at line " << current_line_ 
       << ", column " << current_column_ << ": " << message;
    throw LexicalError(ss.str());
}

/* ========================================================================== */
/* 辅助函数实现 */
/* ========================================================================== */

bool Lexer::IsDigit(int c) {
    return c >= '0' && c <= '9';
}

bool Lexer::IsAlpha(int c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool Lexer::IsAlnum(int c) {
    return IsAlpha(c) || IsDigit(c);
}

bool Lexer::IsSpace(int c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\v' || c == '\f';
}
}
}
}

/* ========================================================================== */
/* Lexer 实现 */
/* ========================================================================== */

Lexer::Lexer(std::unique_ptr<InputStream> input, const LexerConfig& config)
    : input_(std::move(input)), config_(config), current_char_(0), 
      line_number_(1), column_number_(1), has_lookahead_(false),
      buffer_(std::make_unique<TokenBuffer>()) {
    
    if (!input_) {
        throw std::invalid_argument("Input stream cannot be null");
    }
    
    // 读取第一个字符
    NextChar();
}

Lexer::Lexer(const std::string& source, std::string_view source_name, const LexerConfig& config)
    : Lexer(std::make_unique<StringInputStream>(source, source_name), config) {
}

Token Lexer::NextToken() {
    if (has_lookahead_) {
        has_lookahead_ = false;
        current_token_ = lookahead_token_;
        return current_token_;
    }
    
    current_token_ = ScanToken();
    return current_token_;
}

Token Lexer::PeekToken() {
    if (!has_lookahead_) {
        // 保存当前状态
        SaveState();
        lookahead_token_ = ScanToken();
        has_lookahead_ = true;
        // 恢复状态
        RestoreState();
    }
    return lookahead_token_;
}

bool Lexer::Check(TokenType expected_type) {
    return GetCurrentToken().GetType() == expected_type;
}

bool Lexer::Match(TokenType expected_type) {
    if (Check(expected_type)) {
        NextToken();
        return true;
    }
    return false;
}

Token Lexer::Expect(TokenType expected_type) {
    if (!Check(expected_type)) {
        std::stringstream ss;
        ss << "Expected " << TokenTypeToString(expected_type) 
           << " but got " << TokenTypeToString(GetCurrentToken().GetType());
        ReportError(ss.str());
    }
    return NextToken();
}

Token Lexer::GetCurrentToken() const {
    return current_token_;
}

bool Lexer::IsAtEnd() const {
    return current_token_.GetType() == TokenType::EndOfSource;
}

Size Lexer::GetCurrentLine() const {
    return line_number_;
}

Size Lexer::GetCurrentColumn() const {
    return column_number_;
}

std::string_view Lexer::GetSourceName() const {
    return input_->GetSourceName();
}

void Lexer::SetDecimalPoint(char dp) {
    config_.decimal_point = dp;
}

char Lexer::GetDecimalPoint() const {
    return config_.decimal_point;
}

/* ========================================================================== */
/* 私有方法实现 */
/* ========================================================================== */

Token Lexer::ScanToken() {
    // 跳过空白符和注释
    SkipWhitespaceAndComments();
    
    // 保存Token开始位置
    TokenPosition start_pos(line_number_, column_number_);
    
    // 检查是否到达末尾
    if (current_char_ == EOZ) {
        return Token::CreateEndOfSource(start_pos);
    }
    
    char c = current_char_;
    
    // 数字
    if (IsDigit(c) || (c == '.' && IsDigit(PeekChar()))) {
        return ScanNumber(start_pos);
    }
    
    // 字符串
    if (c == '"' || c == '\'') {
        return ScanString(c, start_pos);
    }
    
    // 长字符串 [[ 或 [====[
    if (c == '[') {
        int sep_count = CheckLongStringDelimiter();
        if (sep_count >= 0) {
            return ScanLongString(sep_count, start_pos);
        }
    }
    
    // 标识符和关键字
    if (IsAlpha(c)) {
        return ScanNameOrKeyword(start_pos);
    }
    
    // 单字符操作符和分隔符
    NextChar(); // 消费字符
    
    switch (c) {
        case '+': return Token::CreateOperator(TokenType::Plus, start_pos.line, start_pos.column);
        case '-': 
            // 检查是否是注释 --
            if (current_char_ == '-') {
                return ScanComment(start_pos);
            }
            return Token::CreateOperator(TokenType::Minus, start_pos.line, start_pos.column);
        case '*': return Token::CreateOperator(TokenType::Multiply, start_pos.line, start_pos.column);
        case '/': return Token::CreateOperator(TokenType::Divide, start_pos.line, start_pos.column);
        case '%': return Token::CreateOperator(TokenType::Modulo, start_pos.line, start_pos.column);
        case '^': return Token::CreateOperator(TokenType::Power, start_pos.line, start_pos.column);
        case '#': return Token::CreateOperator(TokenType::Length, start_pos.line, start_pos.column);
        case '(': return Token::CreateDelimiter(TokenType::LeftParen, start_pos.line, start_pos.column);
        case ')': return Token::CreateDelimiter(TokenType::RightParen, start_pos.line, start_pos.column);
        case '{': return Token::CreateDelimiter(TokenType::LeftBrace, start_pos.line, start_pos.column);
        case '}': return Token::CreateDelimiter(TokenType::RightBrace, start_pos.line, start_pos.column);
        case '[': return Token::CreateDelimiter(TokenType::LeftBracket, start_pos.line, start_pos.column);
        case ']': return Token::CreateDelimiter(TokenType::RightBracket, start_pos.line, start_pos.column);
        case ';': return Token::CreateDelimiter(TokenType::Semicolon, start_pos.line, start_pos.column);
        case ',': return Token::CreateDelimiter(TokenType::Comma, start_pos.line, start_pos.column);
        
        // 多字符操作符
        case '<':
            if (current_char_ == '=') {
                NextChar();
                return Token::CreateOperator(TokenType::LessEqual, start_pos.line, start_pos.column);
            }
            return Token::CreateOperator(TokenType::Less, start_pos.line, start_pos.column);
        case '>':
            if (current_char_ == '=') {
                NextChar();
                return Token::CreateOperator(TokenType::GreaterEqual, start_pos.line, start_pos.column);
            }
            return Token::CreateOperator(TokenType::Greater, start_pos.line, start_pos.column);
        case '=':
            if (current_char_ == '=') {
                NextChar();
                return Token::CreateOperator(TokenType::Equal, start_pos.line, start_pos.column);
            }
            return Token::CreateOperator(TokenType::Assign, start_pos.line, start_pos.column);
        case '~':
            if (current_char_ == '=') {
                NextChar();
                return Token::CreateOperator(TokenType::NotEqual, start_pos.line, start_pos.column);
            }
            ReportError("Invalid character '~'");
        case '.':
            if (current_char_ == '.') {
                NextChar();
                if (current_char_ == '.') {
                    NextChar();
                    return Token::CreateOperator(TokenType::Concat, start_pos.line, start_pos.column);
                }
                return Token::CreateOperator(TokenType::Concat, start_pos.line, start_pos.column);
            }
            return Token::CreateOperator(TokenType::Dot, start_pos.line, start_pos.column);
        case ':':
            if (current_char_ == ':') {
                NextChar();
                return Token::CreateOperator(TokenType::Label, start_pos.line, start_pos.column);
            }
            return Token::CreateOperator(TokenType::Colon, start_pos.line, start_pos.column);
            
        default:
            std::stringstream ss;
            ss << "Unexpected character '" << c << "' (ASCII " << static_cast<int>(c) << ")";
            ReportError(ss.str());
    }
}

void Lexer::SkipWhitespaceAndComments() {
    while (true) {
        // 跳过空白符
        while (IsWhitespace(current_char_)) {
            NextChar();
        }
        
        // 检查单行注释
        if (current_char_ == '-' && PeekChar() == '-') {
            NextChar(); // 跳过第一个 -
            NextChar(); // 跳过第二个 -
            
            // 检查是否是长注释
            int sep_count = CheckLongStringDelimiter();
            if (sep_count >= 0) {
                ScanLongComment(sep_count);
            } else {
                ScanLineComment();
            }
            continue;
        }
        
        break;
    }
}

Token Lexer::ScanNumber(const TokenPosition& start_pos) {
    buffer_->Clear();
    
    // 检查十六进制数 0x 或 0X
    if (current_char_ == '0' && (PeekChar() == 'x' || PeekChar() == 'X')) {
        return ScanHexNumber(start_pos);
    }
    
    // 扫描整数部分
    while (IsDigit(current_char_)) {
        buffer_->Append(current_char_);
        NextChar();
    }
    
    // 检查小数点
    if (current_char_ == config_.decimal_point) {
        buffer_->Append(current_char_);
        NextChar();
        
        // 扫描小数部分
        while (IsDigit(current_char_)) {
            buffer_->Append(current_char_);
            NextChar();
        }
    }
    
    // 检查指数部分
    if (current_char_ == 'e' || current_char_ == 'E') {
        buffer_->Append(current_char_);
        NextChar();
        
        // 可选的符号
        if (current_char_ == '+' || current_char_ == '-') {
            buffer_->Append(current_char_);
            NextChar();
        }
        
        // 指数数字
        if (!IsDigit(current_char_)) {
            ReportError("Invalid number format: missing digits in exponent");
        }
        
        while (IsDigit(current_char_)) {
            buffer_->Append(current_char_);
            NextChar();
        }
    }
    
    // 转换为数值
    std::string number_str = buffer_->ToString();
    double value = std::stod(number_str);
    
    return Token::CreateNumber(value, start_pos.line, start_pos.column);
}

Token Lexer::ScanHexNumber(const TokenPosition& start_pos) {
    buffer_->Clear();
    
    // 跳过 '0x' 或 '0X'
    NextChar(); // 跳过 '0'
    NextChar(); // 跳过 'x' 或 'X'
    
    if (!IsHexDigit(current_char_)) {
        ReportError("Invalid hexadecimal number: missing digits after '0x'");
    }
    
    // 扫描十六进制数字
    while (IsHexDigit(current_char_)) {
        buffer_->Append(current_char_);
        NextChar();
    }
    
    // 转换十六进制字符串为数值
    std::string hex_str = buffer_->ToString();
    double value = static_cast<double>(std::stoul(hex_str, nullptr, 16));
    
    return Token::CreateNumber(value, start_pos.line, start_pos.column);
}

[[noreturn]] void Lexer::ReportError(const std::string& message) {
    TokenPosition pos(line_number_, column_number_);
    throw LexicalError(message, pos, std::string(GetSourceName()));
}

// 辅助方法
char Lexer::NextChar() {
    if (current_char_ != EOZ) {
        if (current_char_ == '\n') {
            ++line_number_;
            column_number_ = 1;
        } else {
            ++column_number_;
        }
        
        int next = input_->NextChar();
        current_char_ = (next == -1) ? EOZ : static_cast<char>(next);
    }
    return current_char_;
}

char Lexer::PeekChar() const {
    // 这需要输入流支持前瞻
    // 简化实现：保存状态并恢复
    // 在实际实现中可能需要缓冲机制
    return '\0'; // 临时实现
}

int Lexer::CheckLongStringDelimiter() {
    // 临时实现，返回-1表示不是长字符串格式
    return -1;
}

void Lexer::SaveState() {
    // 保存词法分析器状态以支持前瞻
    saved_char_ = current_char_;
    saved_line_ = line_number_;
    saved_column_ = column_number_;
    saved_position_ = input_->GetPosition();
}

void Lexer::RestoreState() {
    // 恢复词法分析器状态
    current_char_ = saved_char_;
    line_number_ = saved_line_;
    column_number_ = saved_column_;
    // 注意：这需要输入流支持位置重置
    // input_->SetPosition(saved_position_);
}

} // namespace lua_cpp