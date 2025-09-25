/**
 * @file parser_error_recovery.cpp
 * @brief 增强的Parser错误恢复系统实现
 * @date 2025-09-25
 */

#include "parser_error_recovery.h"
#include <algorithm>
#include <sstream>
#include <iostream>
#include <iomanip>

namespace lua_cpp {

/* ========================================================================== */
/* EnhancedSyntaxError 实现 */
/* ========================================================================== */

EnhancedSyntaxError::EnhancedSyntaxError(const std::string& message, 
                                       ErrorSeverity severity,
                                       const SourcePosition& position,
                                       ErrorCategory category,
                                       const std::string& suggestion)
    : SyntaxError(message), severity_(severity), category_(category), suggestion_(suggestion), position_(position) {
}

void EnhancedSyntaxError::AddContext(const std::string& context_line) {
    context_.push_back(context_line);
}

void EnhancedSyntaxError::SetSuggestions(const std::vector<std::string>& suggestions) {
    suggestions_ = suggestions;
}

std::string EnhancedSyntaxError::FormatError() const {
    std::stringstream ss;
    
    // 错误等级
    const char* severity_str = "";
    switch (severity_) {
        case ErrorSeverity::Info: severity_str = "Info"; break;
        case ErrorSeverity::Warning: severity_str = "Warning"; break;
        case ErrorSeverity::Error: severity_str = "Error"; break;
        case ErrorSeverity::Fatal: severity_str = "Fatal"; break;
    }
    
    ss << "[" << severity_str << "] " << GetWhat();
    
    if (!suggestion_.empty()) {
        ss << "\nSuggestion: " << suggestion_;
    }
    
    if (!context_.empty()) {
        ss << "\nContext:";
        for (const auto& line : context_) {
            ss << "\n  " << line;
        }
    }
    
    return ss.str();
}

/* ========================================================================== */
/* ErrorCollector 实现 */
/* ========================================================================== */

ErrorCollector::ErrorCollector(size_t max_errors) : max_errors_(max_errors) {
}

void ErrorCollector::AddError(std::unique_ptr<EnhancedSyntaxError> error) {
    if (errors_.size() < max_errors_) {
        errors_.push_back(std::move(error));
    }
}

void ErrorCollector::AddError(const std::string& message, 
                             const SourcePosition& position,
                             ErrorSeverity severity,
                             const std::string& suggestion) {
    auto error = std::make_unique<EnhancedSyntaxError>(message, severity, position, ErrorCategory::Syntax, suggestion);
    AddError(std::move(error));
}

size_t ErrorCollector::GetErrorCount(ErrorSeverity min_severity) const {
    return std::count_if(errors_.begin(), errors_.end(),
        [min_severity](const auto& error) {
            return error->GetSeverity() >= min_severity;
        });
}

bool ErrorCollector::HasFatalErrors() const {
    return std::any_of(errors_.begin(), errors_.end(),
        [](const auto& error) {
            return error->GetSeverity() == ErrorSeverity::Fatal;
        });
}

std::string ErrorCollector::FormatAllErrors() const {
    std::stringstream ss;
    for (const auto& error : errors_) {
        ss << error->FormatError() << "\n\n";
    }
    return ss.str();
}

std::string ErrorCollector::GetErrorSummary() const {
    size_t fatal = GetErrorCount(ErrorSeverity::Fatal);
    size_t error = GetErrorCount(ErrorSeverity::Error);
    size_t warning = GetErrorCount(ErrorSeverity::Warning);
    size_t info = GetErrorCount(ErrorSeverity::Info);
    
    std::stringstream ss;
    ss << "Error Summary: ";
    if (fatal > 0) ss << fatal << " fatal, ";
    if (error > 0) ss << error << " error" << (error > 1 ? "s" : "") << ", ";
    if (warning > 0) ss << warning << " warning" << (warning > 1 ? "s" : "") << ", ";
    if (info > 0) ss << info << " info";
    
    return ss.str();
}

/* ========================================================================== */
/* ErrorRecoveryEngine 实现 */
/* ========================================================================== */

ErrorRecoveryEngine::ErrorRecoveryEngine() : max_recovery_attempts_(5) {
    InitializeContextSyncTokens();
    
    // 启用默认恢复策略
    enabled_strategies_.insert(EnhancedRecoveryStrategy::SkipToSynchronization);
    enabled_strategies_.insert(EnhancedRecoveryStrategy::InsertMissingToken);
    enabled_strategies_.insert(EnhancedRecoveryStrategy::ContextualRecovery);
}

void ErrorRecoveryEngine::PushContext(ParseContext context, 
                                     const SourcePosition& position,
                                     const std::vector<TokenType>& sync_tokens,
                                     const std::string& description) {
    context_stack_.emplace_back(context, position, sync_tokens, description);
}

void ErrorRecoveryEngine::PopContext() {
    if (!context_stack_.empty()) {
        context_stack_.pop_back();
    }
}

const RecoveryContext* ErrorRecoveryEngine::GetCurrentContext() const {
    return context_stack_.empty() ? nullptr : &context_stack_.back();
}

EnhancedRecoveryStrategy ErrorRecoveryEngine::SelectRecoveryStrategy(
    TokenType current_token, 
    const std::vector<TokenType>& expected_tokens,
    const SourcePosition& position) const {
    
    // 如果有明确的缺失token，优先插入
    if (IsStrategyEnabled(EnhancedRecoveryStrategy::InsertMissingToken)) {
        TokenType missing = SuggestMissingToken(current_token, expected_tokens);
        if (missing != TokenType::EndOfSource) {
            return EnhancedRecoveryStrategy::InsertMissingToken;
        }
    }
    
    // 检查是否可以上下文恢复
    if (IsStrategyEnabled(EnhancedRecoveryStrategy::ContextualRecovery)) {
        const auto* context = GetCurrentContext();
        if (context && IsSyncToken(current_token)) {
            return EnhancedRecoveryStrategy::ContextualRecovery;
        }
    }
    
    // 默认跳到同步点
    if (IsStrategyEnabled(EnhancedRecoveryStrategy::SkipToSynchronization)) {
        return EnhancedRecoveryStrategy::SkipToSynchronization;
    }
    
    return EnhancedRecoveryStrategy::None;
}

std::vector<TokenType> ErrorRecoveryEngine::GetSyncTokensForContext(ParseContext context) const {
    auto it = context_sync_tokens_.find(context);
    return it != context_sync_tokens_.end() ? it->second : std::vector<TokenType>{};
}

bool ErrorRecoveryEngine::IsSyncToken(TokenType token) const {
    // 通用同步token
    switch (token) {
        case TokenType::Semicolon:
        case TokenType::End:
        case TokenType::Else:
        case TokenType::ElseIf:
        case TokenType::Until:
        case TokenType::EndOfSource:
        case TokenType::Local:
        case TokenType::Function:
        case TokenType::If:
        case TokenType::While:
        case TokenType::For:
        case TokenType::Repeat:
        case TokenType::Do:
        case TokenType::Return:
        case TokenType::Break:
            return true;
        default:
            return false;
    }
}

std::string ErrorRecoveryEngine::GenerateErrorSuggestion(
    TokenType current_token,
    const std::vector<TokenType>& expected_tokens,
    ParseContext context) const {
    
    if (expected_tokens.empty()) {
        return "Check syntax near this location";
    }
    
    std::stringstream ss;
    ss << "Expected ";
    
    if (expected_tokens.size() == 1) {
        ss << "'" << static_cast<int>(expected_tokens[0]) << "'";
    } else {
        for (size_t i = 0; i < expected_tokens.size(); ++i) {
            if (i > 0) {
                if (i == expected_tokens.size() - 1) {
                    ss << " or ";
                } else {
                    ss << ", ";
                }
            }
            ss << "'" << static_cast<int>(expected_tokens[i]) << "'";
        }
    }
    
    ss << " near '" << static_cast<int>(current_token) << "'";
    
    // 添加上下文相关建议
    switch (context) {
        case ParseContext::FunctionDef:
            ss << ". Check function definition syntax.";
            break;
        case ParseContext::ControlFlow:
            ss << ". Check control flow statement syntax.";
            break;
        case ParseContext::Expression:
            ss << ". Check expression syntax.";
            break;
        default:
            break;
    }
    
    return ss.str();
}

TokenType ErrorRecoveryEngine::SuggestMissingToken(
    TokenType current_token,
    const std::vector<TokenType>& expected_tokens) const {
    
    // 常见的缺失token模式
    for (TokenType expected : expected_tokens) {
        switch (expected) {
            case TokenType::RightParen:
                if (current_token != TokenType::LeftParen) return TokenType::RightParen;
                break;
            case TokenType::RightBrace:
                if (current_token != TokenType::LeftBrace) return TokenType::RightBrace;
                break;
            case TokenType::RightBracket:
                if (current_token != TokenType::LeftBracket) return TokenType::RightBracket;
                break;
            case TokenType::End:
                return TokenType::End;
            case TokenType::Then:
                if (current_token == TokenType::If || current_token == TokenType::ElseIf) {
                    return TokenType::Then;
                }
                break;
            case TokenType::Do:
                if (current_token == TokenType::While || current_token == TokenType::For) {
                    return TokenType::Do;
                }
                break;
            default:
                break;
        }
    }
    
    return TokenType::EndOfSource; // 没有明显的缺失token
}

void ErrorRecoveryEngine::InitializeContextSyncTokens() {
    context_sync_tokens_[ParseContext::TopLevel] = {
        TokenType::Function, TokenType::Local, TokenType::EndOfSource
    };
    
    context_sync_tokens_[ParseContext::Block] = {
        TokenType::End, TokenType::Else, TokenType::ElseIf, TokenType::Until,
        TokenType::Local, TokenType::Function, TokenType::Return, TokenType::Break
    };
    
    context_sync_tokens_[ParseContext::Expression] = {
        TokenType::Comma, TokenType::Semicolon, TokenType::RightParen,
        TokenType::RightBrace, TokenType::RightBracket
    };
    
    context_sync_tokens_[ParseContext::Statement] = {
        TokenType::Semicolon, TokenType::End, TokenType::Else, TokenType::ElseIf,
        TokenType::Until, TokenType::Local, TokenType::Function
    };
    
    context_sync_tokens_[ParseContext::FunctionDef] = {
        TokenType::End, TokenType::Function, TokenType::Local
    };
    
    context_sync_tokens_[ParseContext::TableConstructor] = {
        TokenType::RightBrace, TokenType::Comma, TokenType::Semicolon
    };
    
    context_sync_tokens_[ParseContext::ParameterList] = {
        TokenType::RightParen, TokenType::Comma
    };
    
    context_sync_tokens_[ParseContext::ArgumentList] = {
        TokenType::RightParen, TokenType::Comma
    };
    
    context_sync_tokens_[ParseContext::ControlFlow] = {
        TokenType::Then, TokenType::Do, TokenType::End, TokenType::Until
    };
}

bool ErrorRecoveryEngine::IsStrategyEnabled(EnhancedRecoveryStrategy strategy) const {
    return enabled_strategies_.find(strategy) != enabled_strategies_.end();
}

/* ========================================================================== */
/* Lua51ErrorFormatter 实现 */
/* ========================================================================== */

Lua51ErrorFormatter::Lua51ErrorFormatter(bool show_source_context, bool color_output)
    : show_source_context_(show_source_context), color_output_(color_output) {
}

std::string Lua51ErrorFormatter::FormatError(const EnhancedSyntaxError& error, 
                                           const std::string& source_code) const {
    std::stringstream ss;
    
    // Lua 5.1.5风格的错误消息
    ss << FormatLua51Message(error.GetWhat(), error.GetPosition(), "");
    
    // 添加建议
    if (!error.GetSuggestion().empty()) {
        ss << "\n  " << ColorizeText("Suggestion: " + error.GetSuggestion(), "yellow");
    }
    
    // 显示源代码上下文
    if (show_source_context_ && !source_code.empty()) {
        std::string context = FormatSourceContext(error.GetPosition(), source_code);
        if (!context.empty()) {
            ss << "\n" << context;
        }
    }
    
    return ss.str();
}

std::string Lua51ErrorFormatter::FormatErrors(
    const std::vector<std::unique_ptr<EnhancedSyntaxError>>& errors,
    const std::string& source_code) const {
    
    std::stringstream ss;
    for (size_t i = 0; i < errors.size(); ++i) {
        if (i > 0) ss << "\n";
        ss << FormatError(*errors[i], source_code);
    }
    return ss.str();
}

std::string Lua51ErrorFormatter::FormatLua51Message(const std::string& message,
                                                  const SourcePosition& position,
                                                  const std::string& filename) const {
    std::stringstream ss;
    
    // Lua 5.1.5格式: "filename:line: message"
    if (!filename.empty()) {
        ss << filename;
    } else if (!position.filename.empty()) {
        ss << position.filename;
    } else {
        ss << "[string \"...\"]";
    }
    
    ss << ":" << position.line << ": " << message;
    
    return ss.str();
}

std::string Lua51ErrorFormatter::FormatSourceContext(const SourcePosition& position,
                                                   const std::string& source_code,
                                                   size_t context_lines) const {
    std::stringstream ss;
    
    // 提取错误行
    std::string error_line = ExtractSourceLine(source_code, position.line);
    if (error_line.empty()) return "";
    
    // 显示错误行
    ss << std::setw(4) << position.line << " | " << error_line << "\n";
    
    // 显示指示器
    ss << "     | ";
    for (size_t i = 1; i < position.column && i <= error_line.length(); ++i) {
        ss << " ";
    }
    ss << ColorizeText("^", "red");
    
    return ss.str();
}

std::string Lua51ErrorFormatter::ColorizeText(const std::string& text, 
                                            const std::string& color) const {
    if (!color_output_) return text;
    
    // ANSI颜色代码
    std::string color_code;
    if (color == "red") color_code = "\033[31m";
    else if (color == "yellow") color_code = "\033[33m";
    else if (color == "green") color_code = "\033[32m";
    else if (color == "blue") color_code = "\033[34m";
    else if (color == "cyan") color_code = "\033[36m";
    else return text;
    
    return color_code + text + "\033[0m";
}

std::string Lua51ErrorFormatter::GetSeverityColor(ErrorSeverity severity) const {
    switch (severity) {
        case ErrorSeverity::Fatal: return "red";
        case ErrorSeverity::Error: return "red";
        case ErrorSeverity::Warning: return "yellow";
        case ErrorSeverity::Info: return "blue";
        default: return "";
    }
}

std::string Lua51ErrorFormatter::ExtractSourceLine(const std::string& source_code, 
                                                 size_t line_number) const {
    if (source_code.empty() || line_number == 0) return "";
    
    std::istringstream stream(source_code);
    std::string line;
    size_t current_line = 0;
    
    while (std::getline(stream, line)) {
        current_line++;
        if (current_line == line_number) {
            return line;
        }
    }
    
    return "";
}

/* ========================================================================== */
/* ErrorSuggestionGenerator 实现 */
/* ========================================================================== */

ErrorSuggestionGenerator::ErrorSuggestionGenerator() {
    InitializeTokenDescriptions();
    InitializeCommonPatterns();
}

std::string ErrorSuggestionGenerator::GenerateSuggestion(
    TokenType current_token,
    const std::vector<TokenType>& expected_tokens,
    ParseContext context) const {
    
    // 检查常见模式
    std::string pattern_suggestion = DetectCommonPattern("", SourcePosition{});
    if (!pattern_suggestion.empty()) {
        return pattern_suggestion;
    }
    
    // 生成基于token的建议
    std::stringstream ss;
    if (expected_tokens.size() == 1) {
        TokenType expected = expected_tokens[0];
        auto it = token_descriptions_.find(expected);
        if (it != token_descriptions_.end()) {
            ss << "Try adding " << it->second;
        }
    }
    
    return ss.str();
}

std::string ErrorSuggestionGenerator::DetectCommonPattern(
    const std::string& error_message,
    const SourcePosition& position) const {
    
    // 检查常见错误模式
    for (const auto& [pattern, suggestion] : suggestion_patterns_) {
        if (error_message.find(pattern) != std::string::npos) {
            return suggestion;
        }
    }
    
    return "";
}

std::string ErrorSuggestionGenerator::SuggestSpellCorrection(
    const std::string& token_text) const {
    
    // 简单的拼写检查 - 检查Lua关键字
    std::vector<std::string> lua_keywords = {
        "and", "break", "do", "else", "elseif", "end", "false", "for",
        "function", "if", "in", "local", "nil", "not", "or", "repeat",
        "return", "then", "true", "until", "while"
    };
    
    std::string best_match;
    int min_distance = std::numeric_limits<int>::max();
    
    for (const auto& keyword : lua_keywords) {
        int distance = LevenshteinDistance(token_text, keyword);
        if (distance < min_distance && distance <= 2) { // 最多2个字符差异
            min_distance = distance;
            best_match = keyword;
        }
    }
    
    if (!best_match.empty()) {
        return "Did you mean '" + best_match + "'?";
    }
    
    return "";
}

void ErrorSuggestionGenerator::AddSuggestionPattern(const std::string& pattern,
                                                  const std::string& suggestion) {
    suggestion_patterns_[pattern] = suggestion;
}

void ErrorSuggestionGenerator::InitializeTokenDescriptions() {
    token_descriptions_[TokenType::Then] = "'then' keyword";
    token_descriptions_[TokenType::Do] = "'do' keyword";
    token_descriptions_[TokenType::End] = "'end' keyword";
    token_descriptions_[TokenType::RightParen] = "closing parenthesis ')'";
    token_descriptions_[TokenType::RightBrace] = "closing brace '}'";
    token_descriptions_[TokenType::RightBracket] = "closing bracket ']'";
    token_descriptions_[TokenType::Comma] = "comma ','";
    token_descriptions_[TokenType::Semicolon] = "semicolon ';'";
}

void ErrorSuggestionGenerator::InitializeCommonPatterns() {
    suggestion_patterns_["expected 'then'"] = "Add 'then' after the condition in if/elseif statement";
    suggestion_patterns_["expected 'do'"] = "Add 'do' after the condition in while/for statement";
    suggestion_patterns_["expected 'end'"] = "Add 'end' to close the block";
    suggestion_patterns_["unexpected ')'"] = "Check for missing opening parenthesis '('";
    suggestion_patterns_["unexpected '}'"] = "Check for missing opening brace '{'";
    suggestion_patterns_["unexpected ']'"] = "Check for missing opening bracket '['";
}

int ErrorSuggestionGenerator::LevenshteinDistance(const std::string& s1, 
                                                const std::string& s2) const {
    const size_t m = s1.length();
    const size_t n = s2.length();
    
    if (m == 0) return static_cast<int>(n);
    if (n == 0) return static_cast<int>(m);
    
    std::vector<std::vector<int>> d(m + 1, std::vector<int>(n + 1));
    
    for (size_t i = 0; i <= m; ++i) d[i][0] = static_cast<int>(i);
    for (size_t j = 0; j <= n; ++j) d[0][j] = static_cast<int>(j);
    
    for (size_t i = 1; i <= m; ++i) {
        for (size_t j = 1; j <= n; ++j) {
            int cost = (s1[i-1] == s2[j-1]) ? 0 : 1;
            d[i][j] = std::min({
                d[i-1][j] + 1,      // deletion
                d[i][j-1] + 1,      // insertion
                d[i-1][j-1] + cost  // substitution
            });
        }
    }
    
    return d[m][n];
}

} // namespace lua_cpp