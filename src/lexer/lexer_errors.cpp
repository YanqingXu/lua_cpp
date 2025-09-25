/**
 * @file lexer_errors.cpp
 * @brief Lexer错误处理系统实现 - T020 SDD实现
 * @description 完整的词法分析错误分类、错误信息生成和错误恢复机制的实现
 * @author Lua C++ Project
 * @date 2025-09-23
 */

#include "lexer_errors.h"
#include <sstream>
#include <algorithm>
#include <cctype>

namespace lua_cpp {

/* ========================================================================== */
/* ErrorLocation 实现 */
/* ========================================================================== */

ErrorLocation::ErrorLocation(Size line, Size column, Size offset, Size length,
                              std::string_view source_name, std::string_view line_text)
    : line(line), column(column), offset(offset), length(length),
      source_name(source_name), line_text(line_text) {
}

std::string ErrorLocation::GetVisualIndicator() const {
    if (line_text.empty()) {
        return "";
    }
    
    std::stringstream ss;
    ss << line_text << "\n";
    
    // 生成指示箭头
    for (Size i = 1; i < column; ++i) {
        if (i - 1 < line_text.size() && line_text[i - 1] == '\t') {
            ss << '\t';
        } else {
            ss << ' ';
        }
    }
    
    // 添加错误指示符
    for (Size i = 0; i < length && i < 20; ++i) {
        ss << '^';
    }
    
    return ss.str();
}

/* ========================================================================== */
/* LexicalError 实现 */
/* ========================================================================== */

LexicalError::LexicalError(LexicalErrorType error_type, const std::string& message,
                           const ErrorLocation& location, ErrorSeverity severity)
    : LuaError(message, ErrorType::SYNTAX_ERROR), error_type_(error_type),
      location_(location), severity_(severity), suggested_recovery_(InferRecoveryStrategy()) {
}

LexicalError::LexicalError(LexicalErrorType error_type, const std::string& message,
                           const TokenPosition& position, ErrorSeverity severity)
    : LuaError(message, ErrorType::SYNTAX_ERROR), error_type_(error_type),
      location_(position.line, position.column, position.offset, 1, position.source_name),
      severity_(severity), suggested_recovery_(InferRecoveryStrategy()) {
}

std::string LexicalError::GetUserFriendlyMessage() const {
    return ErrorMessageGenerator::GenerateUserMessage(error_type_, what());
}

std::string LexicalError::GetDetailedMessage() const {
    return ErrorMessageGenerator::GenerateDetailedMessage(error_type_, location_, what());
}

std::vector<std::string> LexicalError::GetFixSuggestions() const {
    return ErrorMessageGenerator::GenerateFixSuggestions(error_type_, what());
}

RecoveryStrategy LexicalError::InferRecoveryStrategy() const {
    switch (error_type_) {
        case LexicalErrorType::INVALID_CHARACTER:
            return RecoveryStrategy::SKIP_CHARACTER;
            
        case LexicalErrorType::UNTERMINATED_STRING:
        case LexicalErrorType::UNTERMINATED_LONG_STRING:
        case LexicalErrorType::UNTERMINATED_LONG_COMMENT:
            return RecoveryStrategy::SKIP_TO_NEWLINE;
            
        case LexicalErrorType::INCOMPLETE_HEX_NUMBER:
        case LexicalErrorType::INCOMPLETE_DECIMAL_NUMBER:
        case LexicalErrorType::INCOMPLETE_EXPONENT:
        case LexicalErrorType::INVALID_NUMBER_FORMAT:
            return RecoveryStrategy::SKIP_TO_DELIMITER;
            
        case LexicalErrorType::INVALID_ESCAPE_SEQUENCE:
            return RecoveryStrategy::SKIP_CHARACTER;
            
        case LexicalErrorType::TOKEN_TOO_LONG:
        case LexicalErrorType::LINE_TOO_LONG:
            return RecoveryStrategy::TERMINATE_TOKEN;
            
        case LexicalErrorType::UNEXPECTED_EOF:
            return RecoveryStrategy::STOP_LEXING;
            
        default:
            return RecoveryStrategy::SKIP_CHARACTER;
    }
}

/* ========================================================================== */
/* ErrorCollector 实现 */
/* ========================================================================== */

void ErrorCollector::AddError(const LexicalError& error) {
    if (errors_.size() >= max_errors_) {
        return; // 超过最大错误数量限制
    }
    
    errors_.push_back(error);
    
    if (stop_on_fatal_ && error.GetSeverity() == ErrorSeverity::FATAL) {
        return; // 遇到致命错误，停止收集
    }
}

void ErrorCollector::AddError(LexicalErrorType error_type, const std::string& message,
                              const ErrorLocation& location, ErrorSeverity severity) {
    AddError(LexicalError(error_type, message, location, severity));
}

bool ErrorCollector::HasFatalErrors() const {
    return std::any_of(errors_.begin(), errors_.end(),
                       [](const LexicalError& error) {
                           return error.GetSeverity() == ErrorSeverity::FATAL;
                       });
}

Size ErrorCollector::GetWarningCount() const {
    return GetErrorCount(ErrorSeverity::WARNING);
}

Size ErrorCollector::GetErrorCount(ErrorSeverity severity) const {
    return std::count_if(errors_.begin(), errors_.end(),
                         [severity](const LexicalError& error) {
                             return error.GetSeverity() == severity;
                         });
}

std::vector<LexicalError> ErrorCollector::GetErrors(ErrorSeverity severity) const {
    std::vector<LexicalError> filtered_errors;
    std::copy_if(errors_.begin(), errors_.end(), std::back_inserter(filtered_errors),
                 [severity](const LexicalError& error) {
                     return error.GetSeverity() == severity;
                 });
    return filtered_errors;
}

std::string ErrorCollector::GenerateReport() const {
    if (errors_.empty()) {
        return "No lexical errors found.";
    }
    
    std::stringstream ss;
    ss << "Lexical Analysis Report:\n";
    ss << "========================\n\n";
    
    // 按严重性分组
    auto warnings = GetErrors(ErrorSeverity::WARNING);
    auto errors = GetErrors(ErrorSeverity::ERROR);
    auto fatal_errors = GetErrors(ErrorSeverity::FATAL);
    
    // 报告致命错误
    if (!fatal_errors.empty()) {
        ss << "FATAL ERRORS (" << fatal_errors.size() << "):\n";
        for (const auto& error : fatal_errors) {
            ss << "  " << error.GetDetailedMessage() << "\n";
        }
        ss << "\n";
    }
    
    // 报告错误
    if (!errors.empty()) {
        ss << "ERRORS (" << errors.size() << "):\n";
        for (const auto& error : errors) {
            ss << "  " << error.GetDetailedMessage() << "\n";
        }
        ss << "\n";
    }
    
    // 报告警告
    if (!warnings.empty()) {
        ss << "WARNINGS (" << warnings.size() << "):\n";
        for (const auto& error : warnings) {
            ss << "  " << error.GetDetailedMessage() << "\n";
        }
        ss << "\n";
    }
    
    ss << GenerateSummary();
    return ss.str();
}

std::string ErrorCollector::GenerateSummary() const {
    std::stringstream ss;
    Size warning_count = GetWarningCount();
    Size error_count = GetErrorCount(ErrorSeverity::ERROR);
    Size fatal_count = GetErrorCount(ErrorSeverity::FATAL);
    
    ss << "Summary: ";
    if (fatal_count > 0) {
        ss << fatal_count << " fatal error(s), ";
    }
    if (error_count > 0) {
        ss << error_count << " error(s), ";
    }
    if (warning_count > 0) {
        ss << warning_count << " warning(s)";
    }
    
    if (fatal_count == 0 && error_count == 0 && warning_count == 0) {
        ss << "No issues found.";
    }
    
    return ss.str();
}

/* ========================================================================== */
/* ErrorMessageGenerator 实现 */
/* ========================================================================== */

std::string ErrorMessageGenerator::GenerateUserMessage(LexicalErrorType error_type, 
                                                        const std::string& context) {
    std::stringstream ss;
    
    switch (error_type) {
        case LexicalErrorType::INVALID_CHARACTER:
            ss << "Invalid character";
            if (!context.empty()) ss << ": " << context;
            break;
            
        case LexicalErrorType::UNTERMINATED_STRING:
            ss << "Unterminated string literal";
            break;
            
        case LexicalErrorType::UNTERMINATED_LONG_STRING:
            ss << "Unterminated long string literal";
            break;
            
        case LexicalErrorType::INVALID_ESCAPE_SEQUENCE:
            ss << "Invalid escape sequence";
            if (!context.empty()) ss << ": " << context;
            break;
            
        case LexicalErrorType::INVALID_NUMBER_FORMAT:
            ss << "Invalid number format";
            if (!context.empty()) ss << ": " << context;
            break;
            
        case LexicalErrorType::INCOMPLETE_HEX_NUMBER:
            ss << "Incomplete hexadecimal number";
            break;
            
        case LexicalErrorType::INCOMPLETE_EXPONENT:
            ss << "Incomplete exponent in number";
            break;
            
        case LexicalErrorType::MULTIPLE_DECIMAL_POINTS:
            ss << "Multiple decimal points in number";
            break;
            
        case LexicalErrorType::ESCAPE_SEQUENCE_TOO_LARGE:
            ss << "Numeric escape sequence too large";
            break;
            
        case LexicalErrorType::UNTERMINATED_LONG_COMMENT:
            ss << "Unterminated long comment";
            break;
            
        case LexicalErrorType::TOKEN_TOO_LONG:
            ss << "Token exceeds maximum length";
            break;
            
        case LexicalErrorType::LINE_TOO_LONG:
            ss << "Line exceeds maximum length";
            break;
            
        case LexicalErrorType::UNEXPECTED_EOF:
            ss << "Unexpected end of file";
            break;
            
        default:
            ss << "Lexical error";
            if (!context.empty()) ss << ": " << context;
            break;
    }
    
    return ss.str();
}

std::string ErrorMessageGenerator::GenerateDetailedMessage(LexicalErrorType error_type,
                                                            const ErrorLocation& location,
                                                            const std::string& context) {
    std::stringstream ss;
    
    // 添加位置信息
    if (!location.source_name.empty()) {
        ss << location.source_name << ":";
    }
    ss << location.line << ":" << location.column << ": ";
    
    // 添加严重性级别 (这里简化为ERROR)
    ss << "error: ";
    
    // 添加用户友好消息
    ss << GenerateUserMessage(error_type, context);
    
    // 添加可视化指示
    std::string visual = location.GetVisualIndicator();
    if (!visual.empty()) {
        ss << "\n" << visual;
    }
    
    return ss.str();
}

std::vector<std::string> ErrorMessageGenerator::GenerateFixSuggestions(LexicalErrorType error_type,
                                                                        const std::string& context) {
    std::vector<std::string> suggestions;
    
    switch (error_type) {
        case LexicalErrorType::UNTERMINATED_STRING:
            suggestions.push_back("Add closing quote to terminate the string");
            suggestions.push_back("Use [[ ]] for multi-line strings");
            break;
            
        case LexicalErrorType::UNTERMINATED_LONG_STRING:
            suggestions.push_back("Add matching closing bracket sequence");
            suggestions.push_back("Check that opening and closing delimiters match");
            break;
            
        case LexicalErrorType::INVALID_ESCAPE_SEQUENCE:
            suggestions.push_back("Use valid escape sequences: \\n \\t \\r \\\\ \\\" \\'");
            suggestions.push_back("Use \\ddd for decimal character codes (0-255)");
            break;
            
        case LexicalErrorType::INCOMPLETE_HEX_NUMBER:
            suggestions.push_back("Add hexadecimal digits after 0x");
            suggestions.push_back("Example: 0x1F, 0xABC");
            break;
            
        case LexicalErrorType::INCOMPLETE_EXPONENT:
            suggestions.push_back("Add exponent digits after 'e' or 'E'");
            suggestions.push_back("Example: 1.23e10, 4.56E-7");
            break;
            
        case LexicalErrorType::INVALID_CHARACTER:
            suggestions.push_back("Remove or replace the invalid character");
            suggestions.push_back("Check if you meant to use a different character");
            break;
            
        case LexicalErrorType::TOKEN_TOO_LONG:
            suggestions.push_back("Split the token into smaller parts");
            suggestions.push_back("Consider using shorter identifier names");
            break;
            
        default:
            suggestions.push_back("Check the syntax and try again");
            break;
    }
    
    return suggestions;
}

std::string ErrorMessageGenerator::GetErrorTypeName(LexicalErrorType error_type) {
    switch (error_type) {
        case LexicalErrorType::INVALID_CHARACTER: return "INVALID_CHARACTER";
        case LexicalErrorType::INVALID_UNICODE_SEQUENCE: return "INVALID_UNICODE_SEQUENCE";
        case LexicalErrorType::UNEXPECTED_EOF: return "UNEXPECTED_EOF";
        case LexicalErrorType::INVALID_NUMBER_FORMAT: return "INVALID_NUMBER_FORMAT";
        case LexicalErrorType::INCOMPLETE_HEX_NUMBER: return "INCOMPLETE_HEX_NUMBER";
        case LexicalErrorType::INCOMPLETE_DECIMAL_NUMBER: return "INCOMPLETE_DECIMAL_NUMBER";
        case LexicalErrorType::INCOMPLETE_EXPONENT: return "INCOMPLETE_EXPONENT";
        case LexicalErrorType::INVALID_HEX_EXPONENT: return "INVALID_HEX_EXPONENT";
        case LexicalErrorType::NUMBER_TOO_LARGE: return "NUMBER_TOO_LARGE";
        case LexicalErrorType::MULTIPLE_DECIMAL_POINTS: return "MULTIPLE_DECIMAL_POINTS";
        case LexicalErrorType::UNTERMINATED_STRING: return "UNTERMINATED_STRING";
        case LexicalErrorType::INVALID_ESCAPE_SEQUENCE: return "INVALID_ESCAPE_SEQUENCE";
        case LexicalErrorType::ESCAPE_SEQUENCE_TOO_LARGE: return "ESCAPE_SEQUENCE_TOO_LARGE";
        case LexicalErrorType::UNTERMINATED_LONG_STRING: return "UNTERMINATED_LONG_STRING";
        case LexicalErrorType::INVALID_LONG_STRING_DELIMITER: return "INVALID_LONG_STRING_DELIMITER";
        case LexicalErrorType::UNTERMINATED_LONG_COMMENT: return "UNTERMINATED_LONG_COMMENT";
        case LexicalErrorType::INVALID_COMMENT_DELIMITER: return "INVALID_COMMENT_DELIMITER";
        case LexicalErrorType::IDENTIFIER_TOO_LONG: return "IDENTIFIER_TOO_LONG";
        case LexicalErrorType::INVALID_IDENTIFIER_CHAR: return "INVALID_IDENTIFIER_CHAR";
        case LexicalErrorType::INVALID_OPERATOR: return "INVALID_OPERATOR";
        case LexicalErrorType::INCOMPLETE_OPERATOR: return "INCOMPLETE_OPERATOR";
        case LexicalErrorType::TOKEN_TOO_LONG: return "TOKEN_TOO_LONG";
        case LexicalErrorType::LINE_TOO_LONG: return "LINE_TOO_LONG";
        case LexicalErrorType::INVALID_ENCODING: return "INVALID_ENCODING";
        case LexicalErrorType::MIXED_LINE_ENDINGS: return "MIXED_LINE_ENDINGS";
        case LexicalErrorType::UNKNOWN_ERROR: return "UNKNOWN_ERROR";
        default: return "UNKNOWN";
    }
}

std::string ErrorMessageGenerator::GetSeverityName(ErrorSeverity severity) {
    switch (severity) {
        case ErrorSeverity::WARNING: return "warning";
        case ErrorSeverity::ERROR: return "error";
        case ErrorSeverity::FATAL: return "fatal";
        default: return "unknown";
    }
}

/* ========================================================================== */
/* ErrorRecovery 实现 */
/* ========================================================================== */

template<typename AdvanceFunc>
bool ErrorRecovery::ExecuteRecovery(RecoveryStrategy strategy, int& current_char, AdvanceFunc advance_func) {
    switch (strategy) {
        case RecoveryStrategy::SKIP_CHARACTER:
            advance_func();
            return true;
            
        case RecoveryStrategy::SKIP_TO_DELIMITER:
            return SkipToDelimiter(current_char, advance_func);
            
        case RecoveryStrategy::SKIP_TO_NEWLINE:
            return SkipToChar('\n', current_char, advance_func);
            
        case RecoveryStrategy::SKIP_TO_KEYWORD:
            // 实现跳过到关键字的逻辑
            while (current_char != EOZ && !IsKeywordStart(current_char)) {
                advance_func();
            }
            return current_char != EOZ;
            
        case RecoveryStrategy::INSERT_MISSING_CHAR:
            // 插入缺失字符 - 这里只是标记，实际插入由调用者处理
            return true;
            
        case RecoveryStrategy::REPLACE_CHARACTER:
            // 替换字符 - 跳过当前字符
            advance_func();
            return true;
            
        case RecoveryStrategy::TERMINATE_TOKEN:
            // 强制终止当前Token
            return false;
            
        case RecoveryStrategy::STOP_LEXING:
            // 停止词法分析
            return false;
            
        default:
            return false;
    }
}

template<typename AdvanceFunc>
bool ErrorRecovery::SkipToChar(char target_char, int& current_char, AdvanceFunc advance_func) {
    while (current_char != EOZ && current_char != target_char) {
        advance_func();
    }
    return current_char == target_char;
}

template<typename AdvanceFunc>
bool ErrorRecovery::SkipToDelimiter(int& current_char, AdvanceFunc advance_func) {
    while (current_char != EOZ && !IsDelimiter(current_char)) {
        advance_func();
    }
    return current_char != EOZ;
}

bool ErrorRecovery::IsDelimiter(int ch) {
    return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' ||
           ch == '(' || ch == ')' || ch == '{' || ch == '}' ||
           ch == '[' || ch == ']' || ch == ';' || ch == ',' ||
           ch == '+' || ch == '-' || ch == '*' || ch == '/' ||
           ch == '=' || ch == '<' || ch == '>' || ch == '~';
}

bool ErrorRecovery::IsKeywordStart(int ch) {
    return std::isalpha(ch) || ch == '_';
}

// 显式实例化模板函数
template bool ErrorRecovery::ExecuteRecovery<std::function<void()>>(RecoveryStrategy, int&, std::function<void()>);
template bool ErrorRecovery::SkipToChar<std::function<void()>>(char, int&, std::function<void()>);
template bool ErrorRecovery::SkipToDelimiter<std::function<void()>>(int&, std::function<void()>);

} // namespace lua_cpp