/**
 * @file string_lib.h
 * @brief T027 Lua字符串库头文件
 * 
 * 实现Lua 5.1.5字符串库函数：
 * - 基础操作: len, sub, upper, lower, reverse, rep
 * - 查找替换: find, match, gmatch, gsub
 * - 格式化: format, dump
 * - 字节操作: byte, char
 * - 模式匹配: Lua pattern matching
 * 
 * @author Lua C++ Project Team
 * @date 2025-09-26
 * @version 1.0
 */

#pragma once

#include "stdlib_common.h"
#include <regex>

namespace lua_cpp {
namespace stdlib {

/**
 * @brief Lua字符串库模块
 * 
 * 实现Lua 5.1.5标准的字符串库函数
 */
class StringLibrary : public LibraryModule {
public:
    StringLibrary();
    ~StringLibrary() override = default;
    
    // LibraryModule接口实现
    std::string GetModuleName() const override { return "string"; }
    std::string GetModuleVersion() const override { return "1.0.0"; }
    std::vector<LibFunction> GetFunctions() const override;
    void RegisterModule(EnhancedVirtualMachine* vm) override;
    void Initialize(EnhancedVirtualMachine* vm) override;
    void Cleanup(EnhancedVirtualMachine* vm) override;

private:
    // ====================================================================
    // 字符串库函数声明
    // ====================================================================
    
    // 基础操作
    static LUA_STDLIB_FUNCTION(lua_string_len);
    static LUA_STDLIB_FUNCTION(lua_string_sub);
    static LUA_STDLIB_FUNCTION(lua_string_upper);
    static LUA_STDLIB_FUNCTION(lua_string_lower);
    static LUA_STDLIB_FUNCTION(lua_string_reverse);
    static LUA_STDLIB_FUNCTION(lua_string_rep);
    
    // 查找和匹配
    static LUA_STDLIB_FUNCTION(lua_string_find);
    static LUA_STDLIB_FUNCTION(lua_string_match);
    static LUA_STDLIB_FUNCTION(lua_string_gmatch);
    static LUA_STDLIB_FUNCTION(lua_string_gsub);
    
    // 格式化
    static LUA_STDLIB_FUNCTION(lua_string_format);
    static LUA_STDLIB_FUNCTION(lua_string_dump);
    
    // 字节操作
    static LUA_STDLIB_FUNCTION(lua_string_byte);
    static LUA_STDLIB_FUNCTION(lua_string_char);
    
    // ====================================================================
    // 内部辅助函数
    // ====================================================================
    
    /**
     * @brief 标准化字符串索引（处理负数索引）
     * @param index 原始索引
     * @param str_len 字符串长度
     * @return 标准化后的索引（0基）
     */
    static size_t NormalizeIndex(int index, size_t str_len);
    
    /**
     * @brief 验证索引范围
     * @param start 起始索引
     * @param end 结束索引
     * @param str_len 字符串长度
     * @return 有效的索引范围 {start, end}
     */
    static std::pair<size_t, size_t> ValidateRange(int start, int end, size_t str_len);
    
    /**
     * @brief 将Lua模式转换为C++正则表达式
     * @param lua_pattern Lua模式字符串
     * @return C++正则表达式字符串
     */
    static std::string LuaPatternToRegex(const std::string& lua_pattern);
    
    /**
     * @brief 检查字符是否匹配Lua字符类
     * @param ch 要检查的字符
     * @param char_class 字符类（如 %a, %d 等）
     * @return 是否匹配
     */
    static bool MatchCharClass(char ch, const std::string& char_class);
    
    /**
     * @brief 执行简单的Lua模式匹配
     * @param str 目标字符串
     * @param pattern 模式字符串
     * @param start_pos 开始位置
     * @return 匹配结果 {是否匹配, 匹配开始位置, 匹配结束位置, 捕获组}
     */
    struct MatchResult {
        bool found;
        size_t start_pos;
        size_t end_pos;
        std::vector<std::string> captures;
    };
    static MatchResult SimplePatternMatch(const std::string& str, 
                                        const std::string& pattern, 
                                        size_t start_pos = 0);
    
    /**
     * @brief 格式化字符串（printf风格）
     * @param format 格式字符串
     * @param args 参数列表
     * @return 格式化后的字符串
     */
    static std::string FormatString(const std::string& format, const std::vector<LuaValue>& args);
    
    /**
     * @brief 转换字符串中的转义序列
     * @param str 输入字符串
     * @return 处理转义序列后的字符串
     */
    static std::string ProcessEscapeSequences(const std::string& str);
    
    // Lua字符类定义
    static const std::unordered_map<char, std::function<bool(char)>> lua_char_classes_;
};

} // namespace stdlib
} // namespace lua_cpp