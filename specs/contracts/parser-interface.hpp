/**
 * @file parser-interface.hpp
 * @brief 词法分析和语法分析接口契约
 * @date 2025-09-20
 * @version 1.0.0
 * 
 * 定义Lua源代码解析的接口，包括词法分析、语法分析和AST生成。
 * 必须完全兼容Lua 5.1.5的语法规则和错误处理。
 */

#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <optional>
#include <variant>

namespace lua::parser {

/**
 * @brief 词法单元类型
 * 
 * 完全兼容Lua 5.1.5的词法单元集合
 */
enum class TokenType {
    // 字面量
    TK_NUMBER,          // 数字
    TK_STRING,          // 字符串
    TK_NIL,             // nil
    TK_TRUE,            // true
    TK_FALSE,           // false
    
    // 标识符和关键字
    TK_NAME,            // 标识符
    TK_AND,             // and
    TK_BREAK,           // break
    TK_DO,              // do
    TK_ELSE,            // else
    TK_ELSEIF,          // elseif
    TK_END,             // end
    TK_FOR,             // for
    TK_FUNCTION,        // function
    TK_IF,              // if
    TK_IN,              // in
    TK_LOCAL,           // local
    TK_NOT,             // not
    TK_OR,              // or
    TK_REPEAT,          // repeat
    TK_RETURN,          // return
    TK_THEN,            // then
    TK_UNTIL,           // until
    TK_WHILE,           // while
    
    // 运算符
    TK_CONCAT,          // ..
    TK_DOTS,            // ...
    TK_EQ,              // ==
    TK_GE,              // >=
    TK_LE,              // <=
    TK_NE,              // ~=
    
    // 单字符标记
    TK_PLUS,            // +
    TK_MINUS,           // -
    TK_MULTIPLY,        // *
    TK_DIVIDE,          // /
    TK_MOD,             // %
    TK_POWER,           // ^
    TK_HASH,            // #
    TK_ASSIGN,          // =
    TK_LT,              // <
    TK_GT,              // >
    TK_LPAREN,          // (
    TK_RPAREN,          // )
    TK_LBRACE,          // {
    TK_RBRACE,          // }
    TK_LBRACKET,        // [
    TK_RBRACKET,        // ]
    TK_SEMICOLON,       // ;
    TK_COLON,           // :
    TK_COMMA,           // ,
    TK_DOT,             // .
    
    // 特殊标记
    TK_EOF,             // 文件结束
    TK_ERROR            // 错误标记
};

/**
 * @brief 词法单元
 * 
 * 表示源代码中的一个词法单元，包含类型、值和位置信息
 */
struct Token {
    TokenType type;
    std::string value;
    size_t line;
    size_t column;
    size_t position;        // 在源代码中的绝对位置
    
    Token(TokenType t, std::string v, size_t l, size_t c, size_t p)
        : type(t), value(std::move(v)), line(l), column(c), position(p) {}
};

/**
 * @brief AST节点类型
 * 
 * 表示抽象语法树中的不同节点类型
 */
enum class ASTNodeType {
    // 表达式
    EXPR_NIL,           // nil
    EXPR_TRUE,          // true
    EXPR_FALSE,         // false
    EXPR_NUMBER,        // 数字
    EXPR_STRING,        // 字符串
    EXPR_DOTS,          // ...
    EXPR_IDENTIFIER,    // 标识符
    EXPR_BINARY,        // 二元运算
    EXPR_UNARY,         // 一元运算
    EXPR_FUNCTION,      // 函数定义
    EXPR_CALL,          // 函数调用
    EXPR_INDEX,         // 索引访问
    EXPR_MEMBER,        // 成员访问
    EXPR_TABLE,         // 表构造
    
    // 语句
    STMT_BLOCK,         // 语句块
    STMT_ASSIGN,        // 赋值语句
    STMT_CALL,          // 函数调用语句
    STMT_IF,            // if语句
    STMT_WHILE,         // while循环
    STMT_REPEAT,        // repeat循环
    STMT_FOR_NUM,       // 数值for循环
    STMT_FOR_IN,        // 泛型for循环
    STMT_FUNCTION,      // 函数定义语句
    STMT_LOCAL,         // local语句
    STMT_RETURN,        // return语句
    STMT_BREAK,         // break语句
    STMT_DO,            // do语句
    
    // 其他
    CHUNK,              // 代码块
    FIELD,              // 表字段
    PARAMETER_LIST,     // 参数列表
    ARGUMENT_LIST       // 参数列表
};

/**
 * @brief AST节点基类
 * 
 * 所有AST节点的基类，提供类型信息和位置信息
 */
class ASTNode {
public:
    ASTNodeType type;
    size_t line;
    size_t column;
    
    explicit ASTNode(ASTNodeType t, size_t l = 0, size_t c = 0) 
        : type(t), line(l), column(c) {}
    
    virtual ~ASTNode() = default;
    
    // 访问者模式支持
    virtual void accept(class ASTVisitor& visitor) = 0;
};

using ASTNodePtr = std::unique_ptr<ASTNode>;

/**
 * @brief 解析错误信息
 * 
 * 描述语法解析过程中遇到的错误
 */
struct ParseError {
    std::string message;
    size_t line;
    size_t column;
    size_t position;
    
    ParseError(std::string msg, size_t l, size_t c, size_t p)
        : message(std::move(msg)), line(l), column(c), position(p) {}
};

/**
 * @brief 词法分析器接口
 * 
 * 将源代码转换为词法单元序列
 */
class ILexer {
public:
    virtual ~ILexer() = default;
    
    /**
     * @brief 设置输入源代码
     * @param source 源代码字符串
     * @param filename 文件名（用于错误报告）
     */
    virtual void set_input(std::string_view source, std::string_view filename = "") = 0;
    
    /**
     * @brief 获取下一个词法单元
     * @return 词法单元，如果到达文件结束则返回TK_EOF
     */
    virtual Token next_token() = 0;
    
    /**
     * @brief 预览下一个词法单元而不消费它
     * @return 下一个词法单元
     */
    virtual Token peek_token() = 0;
    
    /**
     * @brief 获取当前位置信息
     * @return 行号、列号和绝对位置
     */
    virtual std::tuple<size_t, size_t, size_t> get_position() const = 0;
    
    /**
     * @brief 检查是否有词法错误
     * @return 如果有错误则返回错误信息
     */
    virtual std::optional<ParseError> get_error() const = 0;
    
    /**
     * @brief 重置词法分析器状态
     */
    virtual void reset() = 0;
};

/**
 * @brief 语法分析器接口
 * 
 * 将词法单元序列转换为抽象语法树
 */
class IParser {
public:
    virtual ~IParser() = default;
    
    /**
     * @brief 设置词法分析器
     * @param lexer 词法分析器实例
     */
    virtual void set_lexer(std::unique_ptr<ILexer> lexer) = 0;
    
    /**
     * @brief 解析完整的Lua代码块
     * @return 根AST节点，如果解析失败则返回nullptr
     */
    virtual ASTNodePtr parse_chunk() = 0;
    
    /**
     * @brief 解析表达式
     * @return 表达式AST节点
     */
    virtual ASTNodePtr parse_expression() = 0;
    
    /**
     * @brief 解析语句
     * @return 语句AST节点
     */
    virtual ASTNodePtr parse_statement() = 0;
    
    /**
     * @brief 获取解析错误列表
     * @return 所有解析错误
     */
    virtual std::vector<ParseError> get_errors() const = 0;
    
    /**
     * @brief 检查是否有解析错误
     * @return 是否存在错误
     */
    virtual bool has_errors() const = 0;
    
    /**
     * @brief 重置解析器状态
     */
    virtual void reset() = 0;
};

/**
 * @brief AST访问者接口
 * 
 * 用于遍历和处理AST的访问者模式接口
 */
class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;
    
    // 表达式访问
    virtual void visit_nil_expr(class NilExpr& node) = 0;
    virtual void visit_boolean_expr(class BooleanExpr& node) = 0;
    virtual void visit_number_expr(class NumberExpr& node) = 0;
    virtual void visit_string_expr(class StringExpr& node) = 0;
    virtual void visit_identifier_expr(class IdentifierExpr& node) = 0;
    virtual void visit_binary_expr(class BinaryExpr& node) = 0;
    virtual void visit_unary_expr(class UnaryExpr& node) = 0;
    virtual void visit_function_expr(class FunctionExpr& node) = 0;
    virtual void visit_call_expr(class CallExpr& node) = 0;
    virtual void visit_index_expr(class IndexExpr& node) = 0;
    virtual void visit_member_expr(class MemberExpr& node) = 0;
    virtual void visit_table_expr(class TableExpr& node) = 0;
    
    // 语句访问
    virtual void visit_block_stmt(class BlockStmt& node) = 0;
    virtual void visit_assign_stmt(class AssignStmt& node) = 0;
    virtual void visit_call_stmt(class CallStmt& node) = 0;
    virtual void visit_if_stmt(class IfStmt& node) = 0;
    virtual void visit_while_stmt(class WhileStmt& node) = 0;
    virtual void visit_repeat_stmt(class RepeatStmt& node) = 0;
    virtual void visit_for_num_stmt(class ForNumStmt& node) = 0;
    virtual void visit_for_in_stmt(class ForInStmt& node) = 0;
    virtual void visit_function_stmt(class FunctionStmt& node) = 0;
    virtual void visit_local_stmt(class LocalStmt& node) = 0;
    virtual void visit_return_stmt(class ReturnStmt& node) = 0;
    virtual void visit_break_stmt(class BreakStmt& node) = 0;
    virtual void visit_do_stmt(class DoStmt& node) = 0;
    
    // 其他节点访问
    virtual void visit_chunk(class Chunk& node) = 0;
};

/**
 * @brief 源位置信息
 * 
 * 用于错误报告和调试信息的源代码位置
 */
struct SourceLocation {
    std::string filename;
    size_t line;
    size_t column;
    size_t position;
    
    SourceLocation(std::string file, size_t l, size_t c, size_t p)
        : filename(std::move(file)), line(l), column(c), position(p) {}
    
    std::string to_string() const {
        return filename + ":" + std::to_string(line) + ":" + std::to_string(column);
    }
};

/**
 * @brief 解析结果
 * 
 * 包含解析后的AST和相关信息
 */
struct ParseResult {
    ASTNodePtr root;
    std::vector<ParseError> errors;
    std::vector<std::string> warnings;
    
    bool success() const { return root != nullptr && errors.empty(); }
};

} // namespace lua::parser

/**
 * @brief 语法兼容性保证
 * 
 * 此接口确保完全兼容Lua 5.1.5的语法规则：
 * - 所有关键字和运算符
 * - 完整的语法结构支持
 * - 错误消息格式兼容
 * - 源位置信息准确
 */

/**
 * @brief 解析器性能约束
 */
namespace lua::parser::constraints {
    constexpr size_t MAX_TOKEN_LENGTH = 65536;          // 最大token长度
    constexpr size_t MAX_STRING_LENGTH = 1048576;       // 最大字符串长度
    constexpr size_t MAX_NESTED_DEPTH = 200;            // 最大嵌套深度
    constexpr size_t MAX_LOCAL_VARIABLES = 200;         // 最大局部变量数
}

/**
 * @brief 错误恢复策略
 * 
 * 定义语法错误后的恢复策略，以便继续解析并报告更多错误
 */
namespace lua::parser::recovery {
    enum class Strategy {
        PANIC_MODE,         // 恐慌模式恢复
        PHRASE_LEVEL,       // 短语级恢复
        ERROR_PRODUCTION    // 错误产生式恢复
    };
}