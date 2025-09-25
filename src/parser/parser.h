/**
 * @file parser.h
 * @brief Lua语法分析器定义
 * @description 实现Lua 5.1.5的完整语法分析，构建抽象语法树(AST)
 * @date 2025-09-20
 */

#pragma once

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "parser/ast.h"
#include "lexer/lexer.h"
#include "lexer/token.h"
#include "core/lua_common.h"
#include "core/lua_errors.h"
#include "parser/parser_error_recovery.h"

namespace lua_cpp {

/* ========================================================================== */
/* 操作符优先级定义 */
/* ========================================================================== */

enum class Precedence {
    None        = 0,  // 无优先级
    Assignment  = 1,  // =
    Or          = 2,  // or
    And         = 3,  // and
    Equality    = 4,  // == ~=
    Comparison  = 5,  // < > <= >=
    Concatenate = 6,  // ..
    Term        = 7,  // + -
    Factor      = 8,  // * / %
    Unary       = 9,  // - not #
    Power       = 10, // ^
    Call        = 11, // () [] .
    Primary     = 12  // 字面量、标识符、()
};

/* ========================================================================== */
/* 前向声明（错误类型在lexer_errors.h中定义） */
/* ========================================================================== */

// 使用lexer_errors.h中定义的错误类型

/* ========================================================================== */
/* Parser配置 */
/* ========================================================================== */

struct ParserConfig {
    bool allow_incomplete_programs = false;  // 允许不完整的程序
    bool recover_from_errors = true;         // 错误恢复
    bool track_line_info = true;             // 跟踪行号信息
    bool preserve_comments = false;          // 保留注释
    bool use_enhanced_error_recovery = true; // 使用增强错误恢复
    bool generate_error_suggestions = true;  // 生成错误建议
    Size max_recursion_depth = 1000;        // 最大递归深度
    Size max_expression_depth = 100;        // 最大表达式深度
    Size max_errors = 20;                   // 最大错误数量
};

/* ========================================================================== */
/* Parser状态 */
/* ========================================================================== */

enum class ParserState {
    Ready,          // 准备解析
    Parsing,        // 正在解析
    Error,          // 错误状态
    Completed       // 解析完成
};

/* ========================================================================== */
/* 错误恢复策略（使用lexer_errors.h中定义的） */
/* ========================================================================== */

// RecoveryStrategy已在lexer_errors.h中定义

/* ========================================================================== */
/* Lua语法分析器 */
/* ========================================================================== */

class Parser {
public:
    explicit Parser(std::unique_ptr<Lexer> lexer, const ParserConfig& config = ParserConfig{});
    ~Parser() = default;

    // 禁用拷贝和移动
    Parser(const Parser&) = delete;
    Parser& operator=(const Parser&) = delete;
    Parser(Parser&&) = delete;
    Parser& operator=(Parser&&) = delete;

    /* ======================================================================== */
    /* 解析入口点 */
    /* ======================================================================== */

    // 解析完整程序
    std::unique_ptr<Program> ParseProgram();
    
    // 解析语句
    std::unique_ptr<Statement> ParseStatement();
    
    // 解析表达式
    std::unique_ptr<Expression> ParseExpression();
    
    // 解析表达式（指定优先级）
    std::unique_ptr<Expression> ParseExpression(Precedence min_precedence);

    /* ======================================================================== */
    /* 状态查询 */
    /* ======================================================================== */

    // 检查是否到达文件末尾
    bool IsAtEnd() const;
    
    // 获取当前token
    const Token& GetCurrentToken() const;
    
    // 获取下一个token（不消费）
    const Token& PeekToken() const;
    
    // 获取解析器状态
    ParserState GetState() const { return state_; }
    
    // 获取当前位置
    SourcePosition GetCurrentPosition() const;
    
    // 获取错误计数
    Size GetErrorCount() const { return error_count_; }
    
    // 获取错误收集器
    const ErrorCollector& GetErrorCollector() const { return *error_collector_; }
    
    // 获取所有错误
    std::vector<EnhancedSyntaxError> GetAllErrors() const;

    /* ======================================================================== */
    /* 配置和选项 */
    /* ======================================================================== */

    // 获取配置
    const ParserConfig& GetConfig() const { return config_; }
    
    // 设置错误恢复策略
    void SetRecoveryStrategy(RecoveryStrategy strategy) { recovery_strategy_ = strategy; }
    
    // 获取错误恢复策略
    RecoveryStrategy GetRecoveryStrategy() const { return recovery_strategy_; }
    
    // 获取错误恢复引擎
    const ErrorRecoveryEngine& GetRecoveryEngine() const { return *recovery_engine_; }

private:
    /* ======================================================================== */
    /* Token操作 */
    /* ======================================================================== */

    // 前进到下一个token
    void Advance();
    
    // 匹配并消费指定类型的token
    bool Match(TokenType type);
    
    // 检查当前token是否为指定类型
    bool Check(TokenType type) const;
    
    // 检查当前token是否为指定类型之一
    bool CheckAny(const std::vector<TokenType>& types) const;
    
    // 消费指定类型的token，如果不匹配则抛出错误
    Token Consume(TokenType type, const std::string& error_message);
    
    // 消费指定类型的token，如果不匹配则抛出错误
    Token Consume(TokenType type);

    /* ======================================================================== */
    /* 表达式解析 */
    /* ======================================================================== */

    // 解析主表达式
    std::unique_ptr<Expression> ParsePrimaryExpression();
    
    // 解析后缀表达式
    std::unique_ptr<Expression> ParsePostfixExpression(std::unique_ptr<Expression> expr);
    
    // 解析二元表达式
    std::unique_ptr<Expression> ParseBinaryExpression(std::unique_ptr<Expression> left, 
                                                     Precedence min_precedence);
    
    // 解析一元表达式
    std::unique_ptr<Expression> ParseUnaryExpression();
    
    // 解析函数调用表达式
    std::unique_ptr<Expression> ParseCallExpression(std::unique_ptr<Expression> function);
    
    // 解析索引表达式
    std::unique_ptr<Expression> ParseIndexExpression(std::unique_ptr<Expression> table);
    
    // 解析成员访问表达式
    std::unique_ptr<Expression> ParseMemberExpression(std::unique_ptr<Expression> object);
    
    // 解析方法调用表达式
    std::unique_ptr<Expression> ParseMethodCallExpression(std::unique_ptr<Expression> object);
    
    // 解析表构造表达式
    std::unique_ptr<TableConstructor> ParseTableConstructor();
    
    // 解析表字段
    std::unique_ptr<TableField> ParseTableField();
    
    // 解析函数表达式
    std::unique_ptr<FunctionExpression> ParseFunctionExpression();
    
    // 解析字面量表达式
    std::unique_ptr<Expression> ParseLiteralExpression();

    /* ======================================================================== */
    /* 语句解析 */
    /* ======================================================================== */

    // 解析块语句
    std::unique_ptr<BlockNode> ParseBlock();
    
    // 解析块语句（到指定结束token）
    std::unique_ptr<BlockNode> ParseBlockUntil(const std::vector<TokenType>& end_tokens);
    
    // 解析赋值语句
    std::unique_ptr<Statement> ParseAssignmentStatement(std::vector<std::unique_ptr<Expression>> targets);
    
    // 解析局部变量声明
    std::unique_ptr<LocalDeclaration> ParseLocalDeclaration();
    
    // 解析局部函数定义
    std::unique_ptr<LocalFunctionDefinition> ParseLocalFunctionDefinition();
    
    // 解析函数定义
    std::unique_ptr<FunctionDefinition> ParseFunctionDefinition();
    
    // 解析if语句
    std::unique_ptr<IfStatement> ParseIfStatement();
    
    // 解析while语句
    std::unique_ptr<WhileStatement> ParseWhileStatement();
    
    // 解析repeat语句
    std::unique_ptr<RepeatStatement> ParseRepeatStatement();
    
    // 解析for语句
    std::unique_ptr<Statement> ParseForStatement();
    
    // 解析数值for循环
    std::unique_ptr<NumericForStatement> ParseNumericForStatement(const std::string& variable);
    
    // 解析泛型for循环
    std::unique_ptr<GenericForStatement> ParseGenericForStatement(std::vector<std::string> variables);
    
    // 解析break语句
    std::unique_ptr<BreakStatement> ParseBreakStatement();
    
    // 解析return语句
    std::unique_ptr<ReturnStatement> ParseReturnStatement();
    
    // 解析do语句
    std::unique_ptr<DoStatement> ParseDoStatement();
    
    // 解析表达式语句
    std::unique_ptr<ExpressionStatement> ParseExpressionStatement(std::unique_ptr<Expression> expr);

    /* ======================================================================== */
    /* 辅助解析方法 */
    /* ======================================================================== */

    // 解析参数列表
    std::vector<std::string> ParseParameterList(bool& is_variadic);
    
    // 解析参数表达式列表
    std::vector<std::unique_ptr<Expression>> ParseArgumentList();
    
    // 解析变量列表
    std::vector<std::string> ParseVariableList();
    
    // 解析表达式列表
    std::vector<std::unique_ptr<Expression>> ParseExpressionList();
    
    // 解析可能的变量目标列表
    std::vector<std::unique_ptr<Expression>> ParseVariableTargetList();

    /* ======================================================================== */
    /* 优先级和结合性 */
    /* ======================================================================== */

    // 获取操作符优先级
    Precedence GetPrecedence(TokenType type) const;
    
    // 获取二元操作符
    BinaryOperator GetBinaryOperator(TokenType type) const;
    
    // 获取一元操作符
    UnaryOperator GetUnaryOperator(TokenType type) const;
    
    // 检查是否为右结合操作符
    bool IsRightAssociative(TokenType type) const;
    
    // 检查是否为二元操作符
    bool IsBinaryOperator(TokenType type) const;
    
    // 检查是否为一元操作符
    bool IsUnaryOperator(TokenType type) const;
    
    // 检查是否为赋值目标
    bool IsAssignmentTarget(const Expression* expr) const;

    /* ======================================================================== */
    /* 错误处理和恢复 */
    /* ======================================================================== */

    // 报告语法错误
    void ReportError(const std::string& message);
    
    // 报告增强的语法错误
    void ReportEnhancedError(const EnhancedSyntaxError& error);
    
    // 报告意外的token错误
    void ReportUnexpectedToken(TokenType expected, TokenType actual);
    
    // 报告意外的token错误
    void ReportUnexpectedToken(const std::string& expected, TokenType actual);
    
    // 同步到下一个语句
    void SynchronizeToNextStatement();
    
    // 同步到指定的token类型
    void SynchronizeTo(const std::vector<TokenType>& sync_tokens);
    
    // 尝试恢复错误
    bool TryRecover();
    
    // 尝试增强的错误恢复
    bool TryEnhancedRecover(ErrorContext context);
    
    // 检查递归深度
    void CheckRecursionDepth();
    
    // 检查表达式深度
    void CheckExpressionDepth();
    
    // 创建错误上下文
    ErrorContext CreateErrorContext() const;

    /* ======================================================================== */
    /* 辅助方法 */
    /* ======================================================================== */

    // 创建语法错误位置
    SourcePosition CreateErrorPosition() const;
    
    // 检查是否为语句开始
    bool IsStatementStart(TokenType type) const;
    
    // 检查是否为表达式开始
    bool IsExpressionStart(TokenType type) const;
    
    // 跳过分号
    void SkipOptionalSemicolon();

private:
    // 核心成员
    std::unique_ptr<Lexer> lexer_;
    ParserConfig config_;
    ParserState state_;
    RecoveryStrategy recovery_strategy_;
    
    // Token流状态
    Token current_token_;
    Token peek_token_;
    bool has_peek_token_;
    
    // 错误和调试信息
    Size error_count_;
    Size recursion_depth_;
    Size expression_depth_;
    
    // 增强错误恢复系统
    std::unique_ptr<ErrorCollector> error_collector_;
    std::unique_ptr<ErrorRecoveryEngine> recovery_engine_;
    std::unique_ptr<ErrorSuggestionGenerator> suggestion_generator_;
    std::unique_ptr<Lua51ErrorFormatter> error_formatter_;
    
    // 操作符优先级表
    std::unordered_map<TokenType, Precedence> precedence_table_;
    
    // 二元操作符映射
    std::unordered_map<TokenType, BinaryOperator> binary_operator_map_;
    
    // 一元操作符映射
    std::unordered_map<TokenType, UnaryOperator> unary_operator_map_;
    
    // 右结合操作符集合
    std::unordered_set<TokenType> right_associative_operators_;

    /* ======================================================================== */
    /* 初始化方法 */
    /* ======================================================================== */

    // 初始化操作符表
    void InitializeOperatorTables();
    
    // 初始化解析器状态
    void InitializeState();
};

/* ========================================================================== */
/* 解析器工厂 */
/* ========================================================================== */

class ParserFactory {
public:
    // 从源代码创建解析器
    static std::unique_ptr<Parser> CreateFromSource(const std::string& source,
                                                   const std::string& filename = "",
                                                   const ParserConfig& config = ParserConfig{});
    
    // 从文件创建解析器
    static std::unique_ptr<Parser> CreateFromFile(const std::string& filename,
                                                 const ParserConfig& config = ParserConfig{});
    
    // 从输入流创建解析器
    static std::unique_ptr<Parser> CreateFromStream(std::unique_ptr<InputStream> stream,
                                                   const std::string& filename = "",
                                                   const ParserConfig& config = ParserConfig{});
};

/* ========================================================================== */
/* 便利函数 */
/* ========================================================================== */

// 解析源代码为AST
std::unique_ptr<Program> ParseLuaSource(const std::string& source,
                                       const std::string& filename = "",
                                       const ParserConfig& config = ParserConfig{});

// 解析文件为AST
std::unique_ptr<Program> ParseLuaFile(const std::string& filename,
                                     const ParserConfig& config = ParserConfig{});

// 解析表达式字符串
std::unique_ptr<Expression> ParseLuaExpression(const std::string& expression,
                                              const std::string& filename = "",
                                              const ParserConfig& config = ParserConfig{});

} // namespace lua_cpp