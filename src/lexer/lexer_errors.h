/**
 * @file lexer_errors.h
 * @brief Lexer错误处理系统 - T020 SDD实现
 * @description 完整的词法分析错误分类、错误信息生成和错误恢复机制
 * @author Lua C++ Project
 * @date 2025-09-23
 */

#ifndef LUA_LEXER_ERRORS_H
#define LUA_LEXER_ERRORS_H

#include "token.h"
#include "../core/lua_common.h"
#include "../core/lua_errors.h"
#include <string>
#include <string_view>
#include <vector>
#include <memory>

namespace lua_cpp {

/* ========================================================================== */
/* 词法错误分类 */
/* ========================================================================== */

/**
 * @brief 词法错误类型
 * @description 详细的词法分析错误分类，覆盖所有可能的错误情况
 */
enum class LexicalErrorType {
    // 字符级错误
    INVALID_CHARACTER,          // 非法字符
    INVALID_UNICODE_SEQUENCE,   // 非法Unicode序列
    UNEXPECTED_EOF,             // 意外的文件结束
    
    // 数字格式错误
    INVALID_NUMBER_FORMAT,      // 数字格式错误
    INCOMPLETE_HEX_NUMBER,      // 不完整的十六进制数字
    INCOMPLETE_DECIMAL_NUMBER,  // 不完整的十进制数字
    INCOMPLETE_EXPONENT,        // 不完整的指数
    INVALID_HEX_EXPONENT,       // 非法的十六进制指数
    NUMBER_TOO_LARGE,           // 数字过大
    MULTIPLE_DECIMAL_POINTS,    // 多个小数点
    
    // 字符串错误
    UNTERMINATED_STRING,        // 未终止的字符串
    INVALID_ESCAPE_SEQUENCE,    // 非法转义序列
    ESCAPE_SEQUENCE_TOO_LARGE,  // 转义序列数值过大
    UNTERMINATED_LONG_STRING,   // 未终止的长字符串
    INVALID_LONG_STRING_DELIMITER, // 非法的长字符串分隔符
    
    // 注释错误
    UNTERMINATED_LONG_COMMENT,  // 未终止的长注释
    INVALID_COMMENT_DELIMITER,  // 非法的注释分隔符
    
    // 标识符错误
    IDENTIFIER_TOO_LONG,        // 标识符过长
    INVALID_IDENTIFIER_CHAR,    // 标识符包含非法字符
    
    // 操作符错误
    INVALID_OPERATOR,           // 非法操作符
    INCOMPLETE_OPERATOR,        // 不完整的操作符
    
    // 长度限制错误
    TOKEN_TOO_LONG,             // Token过长
    LINE_TOO_LONG,              // 行过长
    
    // 编码错误
    INVALID_ENCODING,           // 非法编码
    MIXED_LINE_ENDINGS,         // 混合行结束符
    
    // 其他错误
    UNKNOWN_ERROR               // 未知错误
};

/* ========================================================================== */
/* 错误严重性级别 */
/* ========================================================================== */

/**
 * @brief 错误严重性级别
 */
enum class ErrorSeverity {
    WARNING,    // 警告：可以继续处理
    ERROR,      // 错误：影响语法分析但可恢复
    FATAL       // 致命错误：无法继续处理
};

/* ========================================================================== */
/* 错误恢复策略 */
/* ========================================================================== */

/**
 * @brief 错误恢复策略
 */
enum class RecoveryStrategy {
    SKIP_CHARACTER,         // 跳过当前字符
    SKIP_TO_DELIMITER,      // 跳过到下一个分隔符
    SKIP_TO_NEWLINE,        // 跳过到下一行
    SKIP_TO_KEYWORD,        // 跳过到下一个关键字
    INSERT_MISSING_CHAR,    // 插入缺失字符
    REPLACE_CHARACTER,      // 替换字符
    TERMINATE_TOKEN,        // 强制终止当前Token
    STOP_LEXING            // 停止词法分析
};

/* ========================================================================== */
/* 错误位置信息增强 */
/* ========================================================================== */

/**
 * @brief 增强的错误位置信息
 */
struct ErrorLocation {
    Size line;                  // 行号 (1-based)
    Size column;                // 列号 (1-based) 
    Size offset;                // 字符偏移 (0-based)
    Size length;                // 错误范围长度
    std::string source_name;    // 源文件名
    std::string line_text;      // 错误所在行的文本
    
    ErrorLocation() = default;
    ErrorLocation(Size line, Size column, Size offset = 0, Size length = 1,
                  std::string_view source_name = "", std::string_view line_text = "");
    
    /**
     * @brief 获取指示错误位置的可视化字符串
     * @return 带有箭头的错误位置指示
     */
    std::string GetVisualIndicator() const;
};

/* ========================================================================== */
/* 词法错误类 */
/* ========================================================================== */

/**
 * @brief 增强的词法分析错误类
 */
class LexicalError : public LuaError {
public:
    LexicalError(LexicalErrorType error_type, const std::string& message, 
                 const ErrorLocation& location, ErrorSeverity severity = ErrorSeverity::ERROR);
    
    LexicalError(LexicalErrorType error_type, const std::string& message,
                 const TokenPosition& position, ErrorSeverity severity = ErrorSeverity::ERROR);
    
    // 获取错误信息
    LexicalErrorType GetErrorType() const { return error_type_; }
    const ErrorLocation& GetLocation() const { return location_; }
    ErrorSeverity GetSeverity() const { return severity_; }
    RecoveryStrategy GetSuggestedRecovery() const { return suggested_recovery_; }
    
    // 设置恢复策略
    void SetSuggestedRecovery(RecoveryStrategy strategy) { suggested_recovery_ = strategy; }
    
    // 获取用户友好的错误消息
    std::string GetUserFriendlyMessage() const;
    std::string GetDetailedMessage() const;
    
    // 获取修复建议
    std::vector<std::string> GetFixSuggestions() const;

private:
    LexicalErrorType error_type_;
    ErrorLocation location_;
    ErrorSeverity severity_;
    RecoveryStrategy suggested_recovery_;
    
    // 根据错误类型推断恢复策略
    RecoveryStrategy InferRecoveryStrategy() const;
};

/* ========================================================================== */
/* 错误收集器 */
/* ========================================================================== */

/**
 * @brief 错误收集器 - 收集多个错误用于批量报告
 */
class ErrorCollector {
public:
    ErrorCollector() = default;
    
    // 添加错误
    void AddError(const LexicalError& error);
    void AddError(LexicalErrorType error_type, const std::string& message,
                  const ErrorLocation& location, ErrorSeverity severity = ErrorSeverity::ERROR);
    
    // 查询错误
    bool HasErrors() const { return !errors_.empty(); }
    bool HasFatalErrors() const;
    Size GetErrorCount() const { return errors_.size(); }
    Size GetWarningCount() const;
    Size GetErrorCount(ErrorSeverity severity) const;
    
    // 获取错误列表
    const std::vector<LexicalError>& GetErrors() const { return errors_; }
    std::vector<LexicalError> GetErrors(ErrorSeverity severity) const;
    
    // 清空错误
    void Clear() { errors_.clear(); }
    
    // 生成报告
    std::string GenerateReport() const;
    std::string GenerateSummary() const;
    
    // 配置选项
    void SetMaxErrors(Size max_errors) { max_errors_ = max_errors; }
    void SetStopOnFatal(bool stop) { stop_on_fatal_ = stop; }
    
private:
    std::vector<LexicalError> errors_;
    Size max_errors_ = 100;     // 最大错误数量
    bool stop_on_fatal_ = true; // 遇到致命错误时停止
};

/* ========================================================================== */
/* 错误消息生成器 */
/* ========================================================================== */

/**
 * @brief 错误消息生成器
 */
class ErrorMessageGenerator {
public:
    /**
     * @brief 生成用户友好的错误消息
     * @param error_type 错误类型
     * @param context 上下文信息
     * @return 用户友好的错误消息
     */
    static std::string GenerateUserMessage(LexicalErrorType error_type, 
                                           const std::string& context = "");
    
    /**
     * @brief 生成详细的错误消息
     * @param error_type 错误类型
     * @param location 错误位置
     * @param context 上下文信息
     * @return 详细的错误消息
     */
    static std::string GenerateDetailedMessage(LexicalErrorType error_type,
                                               const ErrorLocation& location,
                                               const std::string& context = "");
    
    /**
     * @brief 生成修复建议
     * @param error_type 错误类型
     * @param context 上下文信息
     * @return 修复建议列表
     */
    static std::vector<std::string> GenerateFixSuggestions(LexicalErrorType error_type,
                                                            const std::string& context = "");
    
    /**
     * @brief 获取错误类型的字符串表示
     * @param error_type 错误类型
     * @return 错误类型字符串
     */
    static std::string GetErrorTypeName(LexicalErrorType error_type);
    
    /**
     * @brief 获取严重性级别的字符串表示
     * @param severity 严重性级别
     * @return 严重性级别字符串
     */
    static std::string GetSeverityName(ErrorSeverity severity);
};

/* ========================================================================== */
/* 错误恢复器 */
/* ========================================================================== */

/**
 * @brief 错误恢复器 - 实现错误恢复策略
 */
class ErrorRecovery {
public:
    /**
     * @brief 执行错误恢复
     * @param strategy 恢复策略
     * @param current_char 当前字符
     * @param advance_func 前进函数
     * @return 是否成功恢复
     */
    template<typename AdvanceFunc>
    static bool ExecuteRecovery(RecoveryStrategy strategy, int& current_char, AdvanceFunc advance_func);
    
    /**
     * @brief 跳过到指定字符
     * @param target_char 目标字符
     * @param current_char 当前字符
     * @param advance_func 前进函数
     * @return 是否找到目标字符
     */
    template<typename AdvanceFunc>
    static bool SkipToChar(char target_char, int& current_char, AdvanceFunc advance_func);
    
    /**
     * @brief 跳过到分隔符
     * @param current_char 当前字符
     * @param advance_func 前进函数
     * @return 是否找到分隔符
     */
    template<typename AdvanceFunc>
    static bool SkipToDelimiter(int& current_char, AdvanceFunc advance_func);
    
    /**
     * @brief 判断是否为分隔符
     * @param ch 字符
     * @return 是否为分隔符
     */
    static bool IsDelimiter(int ch);
    
    /**
     * @brief 判断是否为关键字开始字符
     * @param ch 字符
     * @return 是否为关键字开始字符
     */
    static bool IsKeywordStart(int ch);
};

} // namespace lua_cpp

#endif // LUA_LEXER_ERRORS_H