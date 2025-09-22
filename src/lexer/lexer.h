/**
 * @file lexer.h
 * @brief Lua词法分析器定义
 * @description 基于Lua 5.1.5源码的LexState结构，实现完整的词法分析功能
 * @date 2025-09-20
 */

#ifndef LUA_LEXER_H
#define LUA_LEXER_H

#include "token.h"
#include "../core/lua_common.h"
#include "../core/lua_errors.h"
#include <string>
#include <string_view>
#include <memory>
#include <functional>
#include <vector>

namespace lua_cpp {

/* ========================================================================== */
/* 前置声明 */
/* ========================================================================== */

class LuaState;
class InputStream;
class TokenBuffer;

/* ========================================================================== */
/* 词法分析器配置 */
/* ========================================================================== */

/**
 * @brief 词法分析器配置
 * @description 控制词法分析器的行为和特性
 */
struct LexerConfig {
    char decimal_point = '.';        // 十进制小数点字符
    Size tab_width = 8;              // 制表符宽度
    bool allow_unicode_names = true; // 是否允许Unicode标识符
    bool strict_mode = false;        // 严格模式（更严格的错误检查）
    Size max_token_length = 65536;   // 最大Token长度
    Size max_line_length = 1048576;  // 最大行长度

    LexerConfig() = default;
};

/* ========================================================================== */
/* 输入流抽象 */
/* ========================================================================== */

/**
 * @brief 输入流接口
 * @description 对应Lua源码中的ZIO结构，提供字符输入抽象
 */
class InputStream {
public:
    virtual ~InputStream() = default;

    /**
     * @brief 读取下一个字符
     * @return 下一个字符，如果到达末尾则返回-1 (EOZ)
     */
    virtual int NextChar() = 0;

    /**
     * @brief 获取当前位置
     * @return 当前字符偏移位置
     */
    virtual Size GetPosition() const = 0;

    /**
     * @brief 是否到达末尾
     * @return 如果到达末尾则返回true
     */
    virtual bool IsAtEnd() const = 0;

    /**
     * @brief 获取源文件名
     * @return 源文件名
     */
    virtual std::string_view GetSourceName() const = 0;
};

/**
 * @brief 字符串输入流
 * @description 从字符串读取字符的输入流实现
 */
class StringInputStream : public InputStream {
public:
    explicit StringInputStream(const std::string& source, std::string_view source_name = "");
    explicit StringInputStream(std::string&& source, std::string_view source_name = "");

    int NextChar() override;
    Size GetPosition() const override;
    bool IsAtEnd() const override;
    std::string_view GetSourceName() const override;

private:
    std::string source_;
    std::string source_name_;
    Size position_;
};

/**
 * @brief 文件输入流
 * @description 从文件读取字符的输入流实现
 */
class FileInputStream : public InputStream {
public:
    explicit FileInputStream(const std::string& filename);
    ~FileInputStream() override;

    int NextChar() override;
    Size GetPosition() const override;
    bool IsAtEnd() const override;
    std::string_view GetSourceName() const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/* ========================================================================== */
/* Token缓冲区 */
/* ========================================================================== */

/**
 * @brief Token缓冲区
 * @description 对应Lua源码中的Mbuffer，用于构建Token内容
 */
class TokenBuffer {
public:
    TokenBuffer();
    explicit TokenBuffer(Size initial_capacity);
    ~TokenBuffer() = default;

    // 禁止复制，允许移动
    TokenBuffer(const TokenBuffer&) = delete;
    TokenBuffer& operator=(const TokenBuffer&) = delete;
    TokenBuffer(TokenBuffer&&) = default;
    TokenBuffer& operator=(TokenBuffer&&) = default;

    /**
     * @brief 清空缓冲区
     */
    void Clear();

    /**
     * @brief 添加字符
     * @param ch 要添加的字符
     */
    void AppendChar(char ch);

    /**
     * @brief 添加字符串
     * @param str 要添加的字符串
     */
    void AppendString(std::string_view str);

    /**
     * @brief 获取当前内容
     * @return 缓冲区内容的字符串视图
     */
    std::string_view GetContent() const;

    /**
     * @brief 获取内容的副本
     * @return 缓冲区内容的字符串副本
     */
    std::string ToString() const;

    /**
     * @brief 获取当前大小
     * @return 缓冲区中的字符数
     */
    Size GetSize() const;

    /**
     * @brief 获取容量
     * @return 缓冲区容量
     */
    Size GetCapacity() const;

    /**
     * @brief 是否为空
     * @return 如果缓冲区为空则返回true
     */
    bool IsEmpty() const;

    /**
     * @brief 预留空间
     * @param capacity 要预留的容量
     */
    void Reserve(Size capacity);

private:
    std::vector<char> buffer_;
    Size size_;
};

/* ========================================================================== */
/* 词法分析器异常 */
/* ========================================================================== */

/**
 * @brief 词法分析错误
 * @description 词法分析过程中发生的错误
 */
class LexicalError : public LuaError {
public:
    LexicalError(const std::string& message, const TokenPosition& position);
    LexicalError(const std::string& message, Size line, Size column, std::string_view source = "");

    const TokenPosition& GetPosition() const { return position_; }

private:
    TokenPosition position_;
};

/* ========================================================================== */
/* 主词法分析器类 */
/* ========================================================================== */

/**
 * @brief Lua词法分析器
 * @description 对应Lua源码中的LexState结构，实现完整的词法分析功能
 */
class Lexer {
public:
    /* 构造函数和析构函数 */
    explicit Lexer(std::unique_ptr<InputStream> input, const LexerConfig& config = LexerConfig{});
    explicit Lexer(const std::string& source, std::string_view source_name = "", 
                   const LexerConfig& config = LexerConfig{});
    ~Lexer() = default;

    // 禁止复制，允许移动
    Lexer(const Lexer&) = delete;
    Lexer& operator=(const Lexer&) = delete;
    Lexer(Lexer&&) = default;
    Lexer& operator=(Lexer&&) = default;

    /* 主要接口方法 */
    
    /**
     * @brief 获取下一个Token
     * @return 下一个Token，如果到达末尾则返回EndOfSource
     * @throws LexicalError 如果遇到词法错误
     */
    Token NextToken();

    /**
     * @brief 查看下一个Token但不消费它
     * @return 下一个Token，不改变词法分析器状态
     * @throws LexicalError 如果遇到词法错误
     */
    Token PeekToken();

    /**
     * @brief 检查当前Token是否匹配指定类型
     * @param expected_type 期望的Token类型
     * @return 如果匹配则返回true
     */
    bool Check(TokenType expected_type);

    /**
     * @brief 如果当前Token匹配则消费它
     * @param expected_type 期望的Token类型
     * @return 如果匹配并消费则返回true
     */
    bool Match(TokenType expected_type);

    /**
     * @brief 期望一个指定类型的Token
     * @param expected_type 期望的Token类型
     * @return 匹配的Token
     * @throws LexicalError 如果Token类型不匹配
     */
    Token Expect(TokenType expected_type);

    /* 状态查询方法 */

    /**
     * @brief 是否到达源码末尾
     * @return 如果到达末尾则返回true
     */
    bool IsAtEnd() const;

    /**
     * @brief 获取当前行号
     * @return 当前行号 (从1开始)
     */
    Size GetCurrentLine() const { return current_line_; }

    /**
     * @brief 获取当前列号
     * @return 当前列号 (从1开始)
     */
    Size GetCurrentColumn() const { return current_column_; }

    /**
     * @brief 获取当前字符偏移
     * @return 当前字符偏移 (从0开始)
     */
    Size GetCurrentOffset() const;

    /**
     * @brief 获取源文件名
     * @return 源文件名
     */
    std::string_view GetSourceName() const;

    /**
     * @brief 获取当前位置信息
     * @return 当前位置的TokenPosition对象
     */
    TokenPosition GetCurrentPosition() const;

    /**
     * @brief 获取配置
     * @return 词法分析器配置
     */
    const LexerConfig& GetConfig() const { return config_; }

    /* 调试和统计方法 */

    /**
     * @brief 获取已处理的Token数量
     * @return Token数量
     */
    Size GetTokenCount() const { return token_count_; }

    /**
     * @brief 重置统计信息
     */
    void ResetStatistics();

private:
    /* 内部状态 */
    std::unique_ptr<InputStream> input_;   // 输入流
    LexerConfig config_;                   // 配置
    
    int current_char_;                     // 当前字符 (EOZ = -1)
    Size current_line_;                    // 当前行号
    Size current_column_;                  // 当前列号
    Size last_line_;                       // 上一个Token的行号
    
    Token current_token_;                  // 当前Token
    Token lookahead_token_;                // 前瞻Token
    bool has_lookahead_;                   // 是否有前瞻Token
    
    TokenBuffer buffer_;                   // Token构建缓冲区
    Size token_count_;                     // 已处理的Token数量

    /* 字符处理方法 */
    
    /**
     * @brief 读取下一个字符
     */
    void NextChar();

    /**
     * @brief 跳过空白字符
     */
    void SkipWhitespace();

    /**
     * @brief 跳过单行注释
     */
    void SkipLineComment();

    /**
     * @brief 跳过多行注释
     * @param sep_length 分隔符等号数量
     */
    void SkipBlockComment(Size sep_length);

    /**
     * @brief 检测长字符串/注释的分隔符
     * @return 等号数量，如果不是长分隔符则返回-1
     */
    int CheckLongSeparator();

    /* Token识别方法 */

    /**
     * @brief 读取数字Token
     * @return 数字Token
     */
    Token ReadNumber();

    /**
     * @brief 读取字符串Token
     * @param quote 引号字符 (' 或 ")
     * @return 字符串Token
     */
    Token ReadString(char quote);

    /**
     * @brief 读取长字符串Token
     * @param sep_length 分隔符等号数量
     * @return 字符串Token
     */
    Token ReadLongString(Size sep_length);

    /**
     * @brief 读取标识符或关键字Token
     * @return 标识符或关键字Token
     */
    Token ReadName();

    /**
     * @brief 处理转义序列
     * @return 转义后的字符
     */
    char ProcessEscapeSequence();

    /**
     * @brief 读取操作符或分隔符Token
     * @return 操作符或分隔符Token
     */
    Token ReadOperatorOrDelimiter();

    /**
     * @brief 前瞻下一个字符
     * @return 下一个字符，不消费它
     */
    int PeekChar() const;

    /* 实用方法 */

    /**
     * @brief 判断字符是否为字母或下划线
     * @param ch 要判断的字符
     * @return 如果是字母或下划线则返回true
     */
    static bool IsAlpha(int ch);

    /**
     * @brief 判断字符是否为数字
     * @param ch 要判断的字符
     * @return 如果是数字则返回true
     */
    static bool IsDigit(int ch);

    /**
     * @brief 判断字符是否为十六进制数字
     * @param ch 要判断的字符
     * @return 如果是十六进制数字则返回true
     */
    static bool IsHexDigit(int ch);

    /**
     * @brief 判断字符是否为字母数字或下划线
     * @param ch 要判断的字符
     * @return 如果是字母数字或下划线则返回true
     */
    static bool IsAlphaNumeric(int ch);

    /**
     * @brief 判断字符是否为空白字符
     * @param ch 要判断的字符
     * @return 如果是空白字符则返回true
     */
    static bool IsWhitespace(int ch);

    /**
     * @brief 判断字符是否为换行符
     * @param ch 要判断的字符
     * @return 如果是换行符则返回true
     */
    static bool IsNewline(int ch);

    /**
     * @brief 创建错误对象
     * @param message 错误消息
     * @return LexicalError对象
     */
    LexicalError CreateError(const std::string& message) const;

    /**
     * @brief 验证Token长度
     */
    void ValidateTokenLength() const;

    /**
     * @brief 验证行长度
     */
    void ValidateLineLength() const;

    /* 内部实现方法 */
    Token DoNextToken();  // 实际的Token读取实现
};

/* ========================================================================== */
/* 常量定义 */
/* ========================================================================== */

// 缓冲区大小常量
constexpr Size INITIAL_BUFFER_SIZE = 256;     // 初始缓冲区大小
constexpr Size MAX_BUFFER_SIZE = 1024 * 1024; // 最大缓冲区大小

/* ========================================================================== */
/* 实用函数 */
/* ========================================================================== */

/**
 * @brief 从文件创建词法分析器
 * @param filename 文件名
 * @param config 配置
 * @return 词法分析器对象
 */
std::unique_ptr<Lexer> CreateLexerFromFile(const std::string& filename, 
                                            const LexerConfig& config = LexerConfig{});

/**
 * @brief 从字符串创建词法分析器
 * @param source 源代码字符串
 * @param source_name 源文件名
 * @param config 配置
 * @return 词法分析器对象
 */
std::unique_ptr<Lexer> CreateLexerFromString(const std::string& source,
                                              std::string_view source_name = "",
                                              const LexerConfig& config = LexerConfig{});

/**
 * @brief 词法分析整个源码并返回Token序列
 * @param lexer 词法分析器
 * @return Token序列
 */
std::vector<Token> TokenizeAll(Lexer& lexer);

} // namespace lua_cpp

#endif // LUA_LEXER_H