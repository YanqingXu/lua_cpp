/**
 * @file parser_error_recovery.h
 * @brief 增强的Parser错误恢复系统
 * @description 基于Lua 5.1.5兼容性的智能错误检测、报告和恢复机制
 * @date 2025-09-25
 */

#pragma once

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "parser/ast.h"
#include "lexer/token.h"
#include "core/lua_common.h"
#include "core/lua_errors.h"

namespace lua_cpp {

/* ========================================================================== */
/* 增强的错误类型 */
/* ========================================================================== */

/**
 * @brief 语法错误严重等级
 */
enum class ErrorSeverity {
    Info,           // 信息：代码风格建议
    Warning,        // 警告：可能的问题
    Error,          // 错误：语法错误
    Fatal           // 致命：无法恢复的错误
};

/**
 * @brief 错误类别
 */
enum class ErrorCategory {
    Lexical,        // 词法错误
    Syntax,         // 语法错误
    Semantic,       // 语义错误
    Runtime         // 运行时错误
};

/**
 * @brief 增强的语法错误
 */
class EnhancedSyntaxError : public SyntaxError {
public:
    EnhancedSyntaxError(const std::string& message, 
                       ErrorSeverity severity,
                       const SourcePosition& position,
                       ErrorCategory category,
                       const std::string& suggestion = "");

    ErrorSeverity GetSeverity() const { return severity_; }
    ErrorCategory GetCategory() const { return category_; }
    const std::string& GetSuggestion() const { return suggestion_; }
    const std::vector<std::string>& GetContext() const { return context_; }
    const std::vector<std::string>& GetSuggestions() const { return suggestions_; }
    const SourcePosition& GetPosition() const { return position_; }
    
    // 兼容基类的方法
    const char* GetWhat() const { return what(); }
    
    void AddContext(const std::string& context_line);
    void SetSuggestions(const std::vector<std::string>& suggestions);
    std::string FormatError() const;

private:
    ErrorSeverity severity_;
    ErrorCategory category_;
    std::string suggestion_;
    std::vector<std::string> suggestions_;
    SourcePosition position_;
    std::vector<std::string> context_;
};

/* ========================================================================== */
/* 错误恢复上下文 */
/* ========================================================================== */

/**
 * @brief 解析上下文类型
 */
enum class ParseContext {
    TopLevel,           // 顶层
    Block,             // 代码块
    Expression,        // 表达式
    Statement,         // 语句
    FunctionDef,       // 函数定义
    TableConstructor,  // 表构造
    ParameterList,     // 参数列表
    ArgumentList,      // 参数列表
    ControlFlow        // 控制流语句
};

/**
 * @brief 错误恢复上下文
 */
struct RecoveryContext {
    ParseContext context_type;
    SourcePosition start_position;
    std::vector<TokenType> sync_tokens;
    std::string description;
    
    RecoveryContext(ParseContext type, const SourcePosition& pos, 
                   const std::vector<TokenType>& tokens, const std::string& desc)
        : context_type(type), start_position(pos), sync_tokens(tokens), description(desc) {}
};

/* ========================================================================== */
/* 增强的恢复策略 */
/* ========================================================================== */

/**
 * @brief 扩展的错误恢复策略
 */
enum class EnhancedRecoveryStrategy {
    None,                   // 不恢复
    SkipToken,             // 跳过当前token
    SkipToSynchronization, // 跳到同步点
    InsertMissingToken,    // 插入缺失token
    ReplaceToken,          // 替换错误token
    BacktrackAndRetry,     // 回溯重试
    ContextualRecovery,    // 上下文感知恢复
    PanicMode              // 恐慌模式（大幅跳跃）
};

/* ========================================================================== */
/* 错误收集器 */
/* ========================================================================== */

/**
 * @brief 增强的错误收集器
 */
class ErrorCollector {
public:
    ErrorCollector(size_t max_errors = 50);

    // 添加错误
    void AddError(std::unique_ptr<EnhancedSyntaxError> error);
    void AddError(const std::string& message, const SourcePosition& position,
                  ErrorSeverity severity = ErrorSeverity::Error,
                  const std::string& suggestion = "");

    // 错误查询
    size_t GetErrorCount() const { return errors_.size(); }
    size_t GetErrorCount(ErrorSeverity min_severity) const;
    bool HasFatalErrors() const;
    bool HasErrors() const { return !errors_.empty(); }

    // 获取错误
    const std::vector<std::unique_ptr<EnhancedSyntaxError>>& GetErrors() const { return errors_; }
    std::string FormatAllErrors() const;
    std::string GetErrorSummary() const;

    // 清理
    void Clear() { errors_.clear(); }

private:
    std::vector<std::unique_ptr<EnhancedSyntaxError>> errors_;
    size_t max_errors_;
};

/* ========================================================================== */
/* 错误恢复引擎 */
/* ========================================================================== */

/**
 * @brief 智能错误恢复引擎
 */
class ErrorRecoveryEngine {
public:
    ErrorRecoveryEngine();

    // 上下文管理
    void PushContext(ParseContext context, const SourcePosition& position,
                    const std::vector<TokenType>& sync_tokens,
                    const std::string& description = "");
    void PopContext();
    const RecoveryContext* GetCurrentContext() const;

    // 恢复策略选择
    EnhancedRecoveryStrategy SelectRecoveryStrategy(
        TokenType current_token, 
        const std::vector<TokenType>& expected_tokens,
        const SourcePosition& position) const;

    // 同步点查找
    std::vector<TokenType> GetSyncTokensForContext(ParseContext context) const;
    bool IsSyncToken(TokenType token) const;
    
    // 错误建议生成
    std::string GenerateErrorSuggestion(
        TokenType current_token,
        const std::vector<TokenType>& expected_tokens,
        ParseContext context) const;

    // 缺失token检测
    TokenType SuggestMissingToken(
        TokenType current_token,
        const std::vector<TokenType>& expected_tokens) const;

    // 配置
    void SetMaxRecoveryAttempts(size_t max_attempts) { max_recovery_attempts_ = max_attempts; }
    void EnableStrategy(EnhancedRecoveryStrategy strategy) { enabled_strategies_.insert(strategy); }
    void DisableStrategy(EnhancedRecoveryStrategy strategy) { enabled_strategies_.erase(strategy); }

private:
    std::vector<RecoveryContext> context_stack_;
    std::unordered_set<EnhancedRecoveryStrategy> enabled_strategies_;
    std::unordered_map<ParseContext, std::vector<TokenType>> context_sync_tokens_;
    size_t max_recovery_attempts_;

    // 辅助方法
    void InitializeContextSyncTokens();
    bool IsStrategyEnabled(EnhancedRecoveryStrategy strategy) const;
    int CalculateTokenDistance(TokenType token1, TokenType token2) const;
};

/* ========================================================================== */
/* Lua 5.1.5兼容的错误格式化器 */
/* ========================================================================== */

/**
 * @brief Lua 5.1.5兼容的错误格式化器
 */
class Lua51ErrorFormatter {
public:
    Lua51ErrorFormatter(bool show_source_context = true, 
                       bool color_output = false);

    // 格式化单个错误
    std::string FormatError(const EnhancedSyntaxError& error, 
                           const std::string& source_code = "") const;

    // 格式化错误列表
    std::string FormatErrors(const std::vector<std::unique_ptr<EnhancedSyntaxError>>& errors,
                           const std::string& source_code = "") const;

    // Lua 5.1.5风格的错误消息
    std::string FormatLua51Message(const std::string& message,
                                  const SourcePosition& position,
                                  const std::string& filename = "") const;

    // 源代码上下文显示
    std::string FormatSourceContext(const SourcePosition& position,
                                  const std::string& source_code,
                                  size_t context_lines = 2) const;

    // 配置
    void SetShowSourceContext(bool show) { show_source_context_ = show; }
    void SetColorOutput(bool color) { color_output_ = color; }

private:
    bool show_source_context_;
    bool color_output_;

    // 颜色和格式化辅助方法
    std::string ColorizeText(const std::string& text, const std::string& color) const;
    std::string GetSeverityColor(ErrorSeverity severity) const;
    std::string ExtractSourceLine(const std::string& source_code, size_t line_number) const;
};

/* ========================================================================== */
/* 建议生成器 */
/* ========================================================================== */

/**
 * @brief 智能错误建议生成器
 */
class ErrorSuggestionGenerator {
public:
    ErrorSuggestionGenerator();

    // 生成建议
    std::string GenerateSuggestion(TokenType current_token,
                                 const std::vector<TokenType>& expected_tokens,
                                 ParseContext context) const;

    // 常见错误模式检测
    std::string DetectCommonPattern(const std::string& error_message,
                                  const SourcePosition& position) const;

    // 拼写检查
    std::string SuggestSpellCorrection(const std::string& token_text) const;

    // 添加自定义建议模式
    void AddSuggestionPattern(const std::string& pattern, const std::string& suggestion);

private:
    std::unordered_map<std::string, std::string> suggestion_patterns_;
    std::unordered_map<TokenType, std::string> token_descriptions_;

    void InitializeTokenDescriptions();
    void InitializeCommonPatterns();
    int LevenshteinDistance(const std::string& s1, const std::string& s2) const;
};

} // namespace lua_cpp