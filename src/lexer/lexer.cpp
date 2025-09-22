/**
 * @file lexer.cpp
 * @brief Lua词法分析器实现 - T019 SDD实现
 * @description 基于契约测试的现代C++词法分析器实现
 * @author Lua C++ Project  
 * @date 2025-09-22
 */

#include "lexer.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <fstream>

namespace lua_cpp {

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
        return EOZ;
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
    std::ifstream file;
    std::string filename;
    Size position;
    
    Impl(const std::string& name) : filename(name), position(0) {
        file.open(filename, std::ios::binary);
    }
};

FileInputStream::FileInputStream(const std::string& filename) 
    : impl_(std::make_unique<Impl>(filename)) {
    if (!impl_->file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }
}

FileInputStream::~FileInputStream() = default;

int FileInputStream::NextChar() {
    if (!impl_->file.good()) {
        return EOZ;
    }
    
    int ch = impl_->file.get();
    if (ch != EOF) {
        impl_->position++;
        return ch;
    }
    return EOZ;
}

Size FileInputStream::GetPosition() const {
    return impl_->position;
}

bool FileInputStream::IsAtEnd() const {
    return !impl_->file.good() || impl_->file.eof();
}

std::string_view FileInputStream::GetSourceName() const {
    return impl_->filename;
}

/* ========================================================================== */
/* TokenBuffer 实现 */
/* ========================================================================== */

TokenBuffer::TokenBuffer() : size_(0) {
    buffer_.reserve(INITIAL_BUFFER_SIZE);
}

TokenBuffer::TokenBuffer(Size initial_capacity) : size_(0) {
    buffer_.reserve(initial_capacity);
}

void TokenBuffer::Clear() {
    size_ = 0;
}

void TokenBuffer::AppendChar(char ch) {
    if (size_ >= buffer_.capacity()) {
        buffer_.reserve(buffer_.capacity() * 2);
    }
    if (size_ >= buffer_.size()) {
        buffer_.resize(size_ + 1);
    }
    buffer_[size_++] = ch;
}

void TokenBuffer::AppendString(std::string_view str) {
    for (char ch : str) {
        AppendChar(ch);
    }
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
    : LuaError(message, ErrorType::SYNTAX_ERROR), position_(position) {
}

LexicalError::LexicalError(const std::string& message, Size line, Size column, std::string_view source)
    : LuaError(message, ErrorType::SYNTAX_ERROR), position_(line, column, 0, source) {
}

/* ========================================================================== */
/* Lexer 主要实现 */
/* ========================================================================== */

Lexer::Lexer(std::unique_ptr<InputStream> input, const LexerConfig& config)
    : input_(std::move(input)), config_(config), current_char_(0), 
      current_line_(1), current_column_(1), last_line_(1),
      has_lookahead_(false), token_count_(0) {
    
    // 读取第一个字符
    NextChar();
}

Lexer::Lexer(const std::string& source, std::string_view source_name, const LexerConfig& config)
    : Lexer(std::make_unique<StringInputStream>(source, source_name), config) {
}

Token Lexer::NextToken() {
    if (has_lookahead_) {
        current_token_ = std::move(lookahead_token_);
        has_lookahead_ = false;
    } else {
        current_token_ = DoNextToken();
    }
    
    token_count_++;
    return current_token_;
}

Token Lexer::PeekToken() {
    if (!has_lookahead_) {
        lookahead_token_ = DoNextToken();
        has_lookahead_ = true;
    }
    return lookahead_token_;
}

bool Lexer::Check(TokenType expected_type) {
    Token token = PeekToken();
    return token.GetType() == expected_type;
}

bool Lexer::Match(TokenType expected_type) {
    if (Check(expected_type)) {
        NextToken();
        return true;
    }
    return false;
}

Token Lexer::Expect(TokenType expected_type) {
    Token token = NextToken();
    if (token.GetType() != expected_type) {
        std::stringstream ss;
        ss << "Expected token type " << static_cast<int>(expected_type) 
           << ", but got " << static_cast<int>(token.GetType());
        throw CreateError(ss.str());
    }
    return token;
}

bool Lexer::IsAtEnd() const {
    // 检查输入流是否到达末尾并且没有更多token
    return input_->IsAtEnd() && current_char_ == EOZ;
}

Size Lexer::GetCurrentOffset() const {
    return input_->GetPosition();
}

std::string_view Lexer::GetSourceName() const {
    return input_->GetSourceName();
}

TokenPosition Lexer::GetCurrentPosition() const {
    return TokenPosition(current_line_, current_column_, GetCurrentOffset(), GetSourceName());
}

void Lexer::ResetStatistics() {
    token_count_ = 0;
}

/* ========================================================================== */
/* 核心Token识别实现 */
/* ========================================================================== */

Token Lexer::DoNextToken() {
    // 跳过空白字符和注释
    SkipWhitespace();
    
    // 保存Token开始位置
    TokenPosition start_pos = GetCurrentPosition();
    
    // 文件结束
    if (current_char_ == EOZ) {
        return Token::CreateEndOfSource(start_pos);
    }
    
    // 数字字面量
    if (IsDigit(current_char_)) {
        return ReadNumber();
    }
    
    // 字符串字面量
    if (current_char_ == '"' || current_char_ == '\'') {
        return ReadString(current_char_);
    }
    
    // 长字符串或长注释
    if (current_char_ == '[') {
        int sep_length = CheckLongSeparator();
        if (sep_length >= 0) {
            return ReadLongString(sep_length);
        }
    }
    
    // 标识符或关键字
    if (IsAlpha(current_char_)) {
        return ReadName();
    }
    
    // 单字符和多字符Token
    return ReadOperatorOrDelimiter();
}

Token Lexer::ReadOperatorOrDelimiter() {
    TokenPosition start_pos = GetCurrentPosition();
    char ch = current_char_;
    NextChar();
    
    // 检查多字符操作符
    switch (ch) {
        case '.':
            if (current_char_ == '.') {
                NextChar();
                if (current_char_ == '.') {
                    NextChar();
                    return Token::CreateOperator(TokenType::Dots, start_pos.line, start_pos.column);
                }
                return Token::CreateOperator(TokenType::Concat, start_pos.line, start_pos.column);
            }
            return Token::CreateOperator(static_cast<TokenType>(ch), start_pos.line, start_pos.column);
            
        case '=':
            if (current_char_ == '=') {
                NextChar();
                return Token::CreateOperator(TokenType::Equal, start_pos.line, start_pos.column);
            }
            return Token::CreateOperator(static_cast<TokenType>(ch), start_pos.line, start_pos.column);
            
        case '<':
            if (current_char_ == '=') {
                NextChar();
                return Token::CreateOperator(TokenType::LessEqual, start_pos.line, start_pos.column);
            }
            return Token::CreateOperator(static_cast<TokenType>(ch), start_pos.line, start_pos.column);
            
        case '>':
            if (current_char_ == '=') {
                NextChar();
                return Token::CreateOperator(TokenType::GreaterEqual, start_pos.line, start_pos.column);
            }
            return Token::CreateOperator(static_cast<TokenType>(ch), start_pos.line, start_pos.column);
            
        case '~':
            if (current_char_ == '=') {
                NextChar();
                return Token::CreateOperator(TokenType::NotEqual, start_pos.line, start_pos.column);
            }
            break;
    }
    
    // 分隔符Token
    if (ch == '(' || ch == ')' || ch == '{' || ch == '}' || ch == '[' || ch == ']' ||
        ch == ';' || ch == ',') {
        return Token::CreateDelimiter(static_cast<TokenType>(ch), start_pos.line, start_pos.column);
    }
    
    // 操作符Token
    if (ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '%' || ch == '^' || ch == '#' ||
        ch == '<' || ch == '>' || ch == '=') {
        return Token::CreateOperator(static_cast<TokenType>(ch), start_pos.line, start_pos.column);
    }
    
    // 未识别字符
    std::stringstream ss;
    ss << "Unexpected character: '" << ch << "' (ASCII " << static_cast<int>(ch) << ")";
    throw CreateError(ss.str());
}

/* ========================================================================== */
/* 字符处理方法 */
/* ========================================================================== */

void Lexer::NextChar() {
    if (current_char_ == '\n') {
        current_line_++;
        current_column_ = 1;
    } else if (current_char_ != EOZ) {
        current_column_++;
    }
    
    current_char_ = input_->NextChar();
}

void Lexer::SkipWhitespace() {
    while (IsWhitespace(current_char_)) {
        if (current_char_ == '\n') {
            last_line_ = current_line_;
        }
        NextChar();
    }
    
    // 处理注释
    if (current_char_ == '-' && PeekChar() == '-') {
        SkipLineComment();
        SkipWhitespace(); // 递归跳过注释后的空白
    }
}

int Lexer::PeekChar() const {
    // 简化实现：保存当前状态，读取下一个字符，然后恢复
    // 注意：这是简化版本，实际实现可能需要更复杂的前瞻机制
    return EOZ; // 临时实现
}

void Lexer::SkipLineComment() {
    // 跳过 "--"
    NextChar(); // 跳过第一个 '-'
    NextChar(); // 跳过第二个 '-'
    
    // 跳过到行尾
    while (current_char_ != '\n' && current_char_ != EOZ) {
        NextChar();
    }
}

void Lexer::SkipBlockComment(Size sep_length) {
    // 跳过开始的 "--[...["
    // 寻找对应的结束 "]...]-"
    // 简化实现
    while (current_char_ != EOZ) {
        if (current_char_ == ']') {
            // 检查是否匹配结束符
            int close_sep = CheckLongSeparator();
            if (close_sep == static_cast<int>(sep_length)) {
                return;
            }
        }
        NextChar();
    }
}

int Lexer::CheckLongSeparator() {
    // 检查 [===[ 或 ]===] 形式的分隔符
    // 返回等号数量，如果不是长分隔符则返回-1
    if (current_char_ != '[' && current_char_ != ']') {
        return -1;
    }
    
    char bracket = current_char_;
    NextChar();
    
    int sep_count = 0;
    while (current_char_ == '=') {
        sep_count++;
        NextChar();
    }
    
    if (current_char_ == bracket) {
        NextChar();
        return sep_count;
    }
    
    // 不是长分隔符，需要回退（简化实现中暂时不完善）
    return -1;
}

/* ========================================================================== */
/* Token识别方法的简化实现 */
/* ========================================================================== */

Token Lexer::ReadNumber() {
    TokenPosition start_pos = GetCurrentPosition();
    buffer_.Clear();
    
    // 读取数字部分
    while (IsDigit(current_char_)) {
        buffer_.AppendChar(current_char_);
        NextChar();
    }
    
    // 检查小数点
    if (current_char_ == config_.decimal_point) {
        buffer_.AppendChar(current_char_);
        NextChar();
        
        while (IsDigit(current_char_)) {
            buffer_.AppendChar(current_char_);
            NextChar();
        }
    }
    
    // 检查科学计数法
    if (current_char_ == 'e' || current_char_ == 'E') {
        buffer_.AppendChar(current_char_);
        NextChar();
        
        if (current_char_ == '+' || current_char_ == '-') {
            buffer_.AppendChar(current_char_);
            NextChar();
        }
        
        while (IsDigit(current_char_)) {
            buffer_.AppendChar(current_char_);
            NextChar();
        }
    }
    
    // 转换为数字
    std::string number_str = buffer_.ToString();
    double value = std::stod(number_str);
    
    return Token::CreateNumber(value, start_pos.line, start_pos.column);
}

Token Lexer::ReadString(char quote) {
    TokenPosition start_pos = GetCurrentPosition();
    buffer_.Clear();
    
    NextChar(); // 跳过开始引号
    
    while (current_char_ != quote && current_char_ != EOZ) {
        if (current_char_ == '\\') {
            NextChar();
            char escaped = ProcessEscapeSequence();
            buffer_.AppendChar(escaped);
        } else {
            buffer_.AppendChar(current_char_);
            NextChar();
        }
    }
    
    if (current_char_ != quote) {
        throw CreateError("Unterminated string literal");
    }
    
    NextChar(); // 跳过结束引号
    
    return Token::CreateString(buffer_.ToString(), start_pos.line, start_pos.column);
}

Token Lexer::ReadLongString(Size sep_length) {
    TokenPosition start_pos = GetCurrentPosition();
    buffer_.Clear();
    
    // 跳过开始的换行符
    if (current_char_ == '\n') {
        NextChar();
    }
    
    while (current_char_ != EOZ) {
        if (current_char_ == ']') {
            int close_sep = CheckLongSeparator();
            if (close_sep == static_cast<int>(sep_length)) {
                return Token::CreateString(buffer_.ToString(), start_pos.line, start_pos.column);
            }
        }
        buffer_.AppendChar(current_char_);
        NextChar();
    }
    
    throw CreateError("Unterminated long string");
}

Token Lexer::ReadName() {
    TokenPosition start_pos = GetCurrentPosition();
    buffer_.Clear();
    
    // 读取标识符
    while (IsAlphaNumeric(current_char_)) {
        buffer_.AppendChar(current_char_);
        NextChar();
    }
    
    std::string name = buffer_.ToString();
    
    // 检查是否为关键字
    TokenType keyword = ReservedWords::Lookup(name);
    if (keyword != TokenType::Name) {
        return Token::CreateKeyword(keyword, start_pos.line, start_pos.column);
    }
    
    return Token::CreateName(name, start_pos.line, start_pos.column);
}

char Lexer::ProcessEscapeSequence() {
    switch (current_char_) {
        case 'n': NextChar(); return '\n';
        case 't': NextChar(); return '\t';
        case 'r': NextChar(); return '\r';
        case 'b': NextChar(); return '\b';
        case 'f': NextChar(); return '\f';
        case 'a': NextChar(); return '\a';
        case 'v': NextChar(); return '\v';
        case '\\': NextChar(); return '\\';
        case '"': NextChar(); return '"';
        case '\'': NextChar(); return '\'';
        case '\n': NextChar(); return '\n';
        default:
            char ch = current_char_;
            NextChar();
            return ch;
    }
}

/* ========================================================================== */
/* 实用方法 */
/* ========================================================================== */

bool Lexer::IsAlpha(int ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_';
}

bool Lexer::IsDigit(int ch) {
    return ch >= '0' && ch <= '9';
}

bool Lexer::IsHexDigit(int ch) {
    return IsDigit(ch) || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F');
}

bool Lexer::IsAlphaNumeric(int ch) {
    return IsAlpha(ch) || IsDigit(ch);
}

bool Lexer::IsWhitespace(int ch) {
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n' || ch == '\f' || ch == '\v';
}

bool Lexer::IsNewline(int ch) {
    return ch == '\n' || ch == '\r';
}

LexicalError Lexer::CreateError(const std::string& message) const {
    return LexicalError(message, GetCurrentPosition());
}

void Lexer::ValidateTokenLength() const {
    if (buffer_.GetSize() > config_.max_token_length) {
        throw CreateError("Token too long");
    }
}

void Lexer::ValidateLineLength() const {
    if (current_column_ > config_.max_line_length) {
        throw CreateError("Line too long");
    }
}

/* ========================================================================== */
/* 实用函数 */
/* ========================================================================== */

std::unique_ptr<Lexer> CreateLexerFromFile(const std::string& filename, const LexerConfig& config) {
    auto input = std::make_unique<FileInputStream>(filename);
    return std::make_unique<Lexer>(std::move(input), config);
}

std::unique_ptr<Lexer> CreateLexerFromString(const std::string& source, std::string_view source_name, const LexerConfig& config) {
    return std::make_unique<Lexer>(source, source_name, config);
}

std::vector<Token> TokenizeAll(Lexer& lexer) {
    std::vector<Token> tokens;
    
    while (!lexer.IsAtEnd()) {
        Token token = lexer.NextToken();
        tokens.push_back(token);
        
        if (token.GetType() == TokenType::EndOfSource) {
            break;
        }
    }
    
    return tokens;
}

} // namespace lua_cpp