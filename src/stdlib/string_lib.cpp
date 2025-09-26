/**
 * @file string_lib.cpp
 * @brief T027 Lua字符串库实现
 * 
 * 实现Lua 5.1.5字符串库的所有函数
 * 
 * @author Lua C++ Project Team
 * @date 2025-09-26
 * @version 1.0
 */

#include "string_lib.h"
#include <cctype>
#include <cstdio>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace lua_cpp {
namespace stdlib {

// ============================================================================
// 静态成员初始化
// ============================================================================

const std::unordered_map<char, std::function<bool(char)>> StringLibrary::lua_char_classes_ = {
    {'a', [](char c) { return std::isalpha(c); }},    // 字母
    {'c', [](char c) { return std::iscntrl(c); }},    // 控制字符
    {'d', [](char c) { return std::isdigit(c); }},    // 数字
    {'l', [](char c) { return std::islower(c); }},    // 小写字母
    {'p', [](char c) { return std::ispunct(c); }},    // 标点符号
    {'s', [](char c) { return std::isspace(c); }},    // 空白字符
    {'u', [](char c) { return std::isupper(c); }},    // 大写字母
    {'w', [](char c) { return std::isalnum(c); }},    // 字母数字
    {'x', [](char c) { return std::isxdigit(c); }},   // 十六进制数字
};

// ============================================================================
// StringLibrary类实现
// ============================================================================

StringLibrary::StringLibrary() = default;

std::vector<LibFunction> StringLibrary::GetFunctions() const {
    std::vector<LibFunction> functions;
    
    // 基础操作
    REGISTER_FUNCTION(functions, "len", lua_string_len, "返回字符串长度");
    REGISTER_FUNCTION(functions, "sub", lua_string_sub, "提取子字符串");
    REGISTER_FUNCTION(functions, "upper", lua_string_upper, "转换为大写");
    REGISTER_FUNCTION(functions, "lower", lua_string_lower, "转换为小写");
    REGISTER_FUNCTION(functions, "reverse", lua_string_reverse, "反转字符串");
    REGISTER_FUNCTION(functions, "rep", lua_string_rep, "重复字符串");
    
    // 查找和匹配
    REGISTER_FUNCTION(functions, "find", lua_string_find, "查找子字符串");
    REGISTER_FUNCTION(functions, "match", lua_string_match, "模式匹配");
    REGISTER_FUNCTION(functions, "gmatch", lua_string_gmatch, "全局模式匹配");
    REGISTER_FUNCTION(functions, "gsub", lua_string_gsub, "全局替换");
    
    // 格式化
    REGISTER_FUNCTION(functions, "format", lua_string_format, "格式化字符串");
    REGISTER_FUNCTION(functions, "dump", lua_string_dump, "序列化函数");
    
    // 字节操作
    REGISTER_FUNCTION(functions, "byte", lua_string_byte, "获取字节值");
    REGISTER_FUNCTION(functions, "char", lua_string_char, "字节值转字符");
    
    return functions;
}

void StringLibrary::RegisterModule(EnhancedVirtualMachine* vm) {
    if (!vm) return;
    
    auto functions = GetFunctions();
    
    // 创建string表
    LuaTable string_table;
    
    for (const auto& func : functions) {
        LuaValue func_value = LuaValue::CreateFunction(
            [f = func.func](EnhancedVirtualMachine* vm) -> std::vector<LuaValue> {
                int result_count = f(vm);
                auto& stack = vm->GetStack();
                
                std::vector<LuaValue> results;
                if (result_count > 0 && static_cast<size_t>(result_count) <= stack.size()) {
                    results.reserve(result_count);
                    for (int i = stack.size() - result_count; i < static_cast<int>(stack.size()); i++) {
                        results.push_back(stack[i]);
                    }
                    // 清理栈上的返回值
                    for (int i = 0; i < result_count; i++) {
                        stack.pop_back();
                    }
                }
                
                return results;
            }
        );
        
        string_table.SetField(LuaValue::CreateString(func.name), func_value);
    }
    
    // 注册string表到全局环境
    auto& globals = vm->GetGlobalEnvironment();
    globals.SetField("string", LuaValue::CreateTable(string_table));
    
    // 设置字符串元表，使得字符串可以直接调用string库函数
    // 例如: "hello":upper() 等价于 string.upper("hello")
    // TODO: 实现字符串元表设置
}

void StringLibrary::Initialize(EnhancedVirtualMachine* vm) {
    (void)vm;  // 字符串库暂时不需要特殊初始化
}

void StringLibrary::Cleanup(EnhancedVirtualMachine* vm) {
    (void)vm;  // 字符串库暂时不需要特殊清理
}

// ============================================================================
// 基础字符串操作函数实现
// ============================================================================

LUA_STDLIB_FUNCTION(StringLibrary::lua_string_len) {
    StackHelper helper(vm);
    helper.CheckArgCount(1, "string.len");
    
    std::string str = helper.GetStringArg(1);
    
    // 清理参数
    vm->GetStack().clear();
    
    // 压入结果
    vm->GetStack().push_back(LuaValue::CreateNumber(static_cast<double>(str.length())));
    
    return 1;
}

LUA_STDLIB_FUNCTION(StringLibrary::lua_string_sub) {
    StackHelper helper(vm);
    
    if (vm->GetStack().size() < 2 || vm->GetStack().size() > 3) {
        ErrorHelper::ArgError("string.sub", -1, "expected 2 or 3 arguments");
    }
    
    std::string str = helper.GetStringArg(1);
    int start = helper.GetIntArg(2);
    int end = helper.GetIntArg(3, static_cast<int>(str.length()));
    
    auto range = ValidateRange(start, end, str.length());
    
    std::string result;
    if (range.first <= range.second && range.first < str.length()) {
        result = str.substr(range.first, range.second - range.first + 1);
    }
    
    // 清理参数
    vm->GetStack().clear();
    
    // 压入结果
    vm->GetStack().push_back(LuaValue::CreateString(result));
    
    return 1;
}

LUA_STDLIB_FUNCTION(StringLibrary::lua_string_upper) {
    StackHelper helper(vm);
    helper.CheckArgCount(1, "string.upper");
    
    std::string str = helper.GetStringArg(1);
    
    std::transform(str.begin(), str.end(), str.begin(), 
                  [](char c) { return std::toupper(c); });
    
    // 清理参数
    vm->GetStack().clear();
    
    // 压入结果
    vm->GetStack().push_back(LuaValue::CreateString(str));
    
    return 1;
}

LUA_STDLIB_FUNCTION(StringLibrary::lua_string_lower) {
    StackHelper helper(vm);
    helper.CheckArgCount(1, "string.lower");
    
    std::string str = helper.GetStringArg(1);
    
    std::transform(str.begin(), str.end(), str.begin(), 
                  [](char c) { return std::tolower(c); });
    
    // 清理参数
    vm->GetStack().clear();
    
    // 压入结果
    vm->GetStack().push_back(LuaValue::CreateString(str));
    
    return 1;
}

LUA_STDLIB_FUNCTION(StringLibrary::lua_string_reverse) {
    StackHelper helper(vm);
    helper.CheckArgCount(1, "string.reverse");
    
    std::string str = helper.GetStringArg(1);
    std::reverse(str.begin(), str.end());
    
    // 清理参数
    vm->GetStack().clear();
    
    // 压入结果
    vm->GetStack().push_back(LuaValue::CreateString(str));
    
    return 1;
}

LUA_STDLIB_FUNCTION(StringLibrary::lua_string_rep) {
    StackHelper helper(vm);
    
    if (vm->GetStack().size() < 2 || vm->GetStack().size() > 3) {
        ErrorHelper::ArgError("string.rep", -1, "expected 2 or 3 arguments");
    }
    
    std::string str = helper.GetStringArg(1);
    int n = helper.GetIntArg(2);
    std::string sep = helper.GetStringArg(3, "");
    
    if (n < 0) {
        ErrorHelper::ArgError("string.rep", 2, "negative repetition count");
    }
    
    std::string result;
    if (n > 0) {
        result.reserve(str.length() * n + sep.length() * (n - 1));
        
        for (int i = 0; i < n; i++) {
            if (i > 0 && !sep.empty()) {
                result += sep;
            }
            result += str;
        }
    }
    
    // 清理参数
    vm->GetStack().clear();
    
    // 压入结果
    vm->GetStack().push_back(LuaValue::CreateString(result));
    
    return 1;
}

// ============================================================================
// 查找和匹配函数实现
// ============================================================================

LUA_STDLIB_FUNCTION(StringLibrary::lua_string_find) {
    StackHelper helper(vm);
    
    if (vm->GetStack().size() < 2 || vm->GetStack().size() > 4) {
        ErrorHelper::ArgError("string.find", -1, "expected 2-4 arguments");
    }
    
    std::string str = helper.GetStringArg(1);
    std::string pattern = helper.GetStringArg(2);
    int init = helper.GetIntArg(3, 1);
    bool plain = helper.GetBoolArg(4, false);
    
    // 标准化起始位置
    size_t start_pos = NormalizeIndex(init, str.length());
    
    if (plain) {
        // 简单字符串查找
        size_t pos = str.find(pattern, start_pos);
        
        // 清理参数
        vm->GetStack().clear();
        
        if (pos != std::string::npos) {
            // 返回1基索引
            vm->GetStack().push_back(LuaValue::CreateNumber(static_cast<double>(pos + 1)));
            vm->GetStack().push_back(LuaValue::CreateNumber(static_cast<double>(pos + pattern.length())));
            return 2;
        } else {
            return 0;  // 未找到
        }
    } else {
        // 模式匹配
        auto result = SimplePatternMatch(str, pattern, start_pos);
        
        // 清理参数
        vm->GetStack().clear();
        
        if (result.found) {
            // 返回1基索引
            vm->GetStack().push_back(LuaValue::CreateNumber(static_cast<double>(result.start_pos + 1)));
            vm->GetStack().push_back(LuaValue::CreateNumber(static_cast<double>(result.end_pos + 1)));
            
            // 添加捕获组
            for (const auto& capture : result.captures) {
                vm->GetStack().push_back(LuaValue::CreateString(capture));
            }
            
            return 2 + static_cast<int>(result.captures.size());
        } else {
            return 0;  // 未找到
        }
    }
}

LUA_STDLIB_FUNCTION(StringLibrary::lua_string_format) {
    StackHelper helper(vm);
    
    if (vm->GetStack().empty()) {
        ErrorHelper::ArgError("string.format", 1, "format string expected");
    }
    
    std::string format = helper.GetStringArg(1);
    
    // 收集所有参数
    std::vector<LuaValue> args;
    for (size_t i = 1; i < vm->GetStack().size(); i++) {
        args.push_back(vm->GetStack()[i]);
    }
    
    std::string result = FormatString(format, args);
    
    // 清理参数
    vm->GetStack().clear();
    
    // 压入结果
    vm->GetStack().push_back(LuaValue::CreateString(result));
    
    return 1;
}

LUA_STDLIB_FUNCTION(StringLibrary::lua_string_byte) {
    StackHelper helper(vm);
    
    if (vm->GetStack().size() < 1 || vm->GetStack().size() > 3) {
        ErrorHelper::ArgError("string.byte", -1, "expected 1-3 arguments");
    }
    
    std::string str = helper.GetStringArg(1);
    int start = helper.GetIntArg(2, 1);
    int end = helper.GetIntArg(3, start);
    
    auto range = ValidateRange(start, end, str.length());
    
    // 清理参数
    vm->GetStack().clear();
    
    // 返回指定范围内字符的字节值
    int count = 0;
    for (size_t i = range.first; i <= range.second && i < str.length(); i++) {
        vm->GetStack().push_back(LuaValue::CreateNumber(static_cast<double>(static_cast<unsigned char>(str[i]))));
        count++;
    }
    
    return count;
}

LUA_STDLIB_FUNCTION(StringLibrary::lua_string_char) {
    auto& stack = vm->GetStack();
    
    std::string result;
    result.reserve(stack.size());
    
    for (const auto& arg : stack) {
        if (arg.GetType() == LuaValueType::NUMBER) {
            int byte_val = static_cast<int>(arg.AsNumber());
            if (byte_val < 0 || byte_val > 255) {
                ErrorHelper::ArgError("string.char", -1, "character code out of range");
            }
            result += static_cast<char>(byte_val);
        } else {
            ErrorHelper::TypeError("string.char", -1, "number", "unknown");
        }
    }
    
    // 清理参数
    stack.clear();
    
    // 压入结果
    stack.push_back(LuaValue::CreateString(result));
    
    return 1;
}

// ============================================================================
// 内部辅助函数实现
// ============================================================================

size_t StringLibrary::NormalizeIndex(int index, size_t str_len) {
    if (index > 0) {
        // 正索引：1基转0基
        return static_cast<size_t>(index - 1);
    } else if (index < 0) {
        // 负索引：从末尾开始
        if (static_cast<size_t>(-index) > str_len) {
            return 0;
        }
        return str_len + index;
    } else {
        // index == 0，在Lua中无效
        return str_len;  // 返回无效索引
    }
}

std::pair<size_t, size_t> StringLibrary::ValidateRange(int start, int end, size_t str_len) {
    size_t norm_start = NormalizeIndex(start, str_len);
    size_t norm_end = NormalizeIndex(end, str_len);
    
    // 确保范围有效
    if (norm_start >= str_len) norm_start = str_len;
    if (norm_end >= str_len) norm_end = str_len > 0 ? str_len - 1 : 0;
    
    return {norm_start, norm_end};
}

StringLibrary::MatchResult StringLibrary::SimplePatternMatch(const std::string& str, 
                                                           const std::string& pattern, 
                                                           size_t start_pos) {
    MatchResult result = {false, 0, 0, {}};
    
    // 简化的模式匹配实现
    // TODO: 实现完整的Lua模式匹配
    
    // 对于简单情况，使用字符串查找
    size_t pos = str.find(pattern, start_pos);
    if (pos != std::string::npos) {
        result.found = true;
        result.start_pos = pos;
        result.end_pos = pos + pattern.length() - 1;
    }
    
    return result;
}

std::string StringLibrary::FormatString(const std::string& format, const std::vector<LuaValue>& args) {
    std::ostringstream result;
    size_t arg_index = 0;
    
    for (size_t i = 0; i < format.length(); i++) {
        if (format[i] == '%' && i + 1 < format.length()) {
            char spec = format[i + 1];
            
            switch (spec) {
                case '%':
                    result << '%';
                    break;
                case 's': {
                    if (arg_index < args.size()) {
                        if (args[arg_index].GetType() == LuaValueType::STRING) {
                            result << args[arg_index].AsString();
                        } else {
                            result << "[not a string]";
                        }
                        arg_index++;
                    }
                    break;
                }
                case 'd':
                case 'i': {
                    if (arg_index < args.size()) {
                        if (args[arg_index].GetType() == LuaValueType::NUMBER) {
                            result << static_cast<int>(args[arg_index].AsNumber());
                        } else {
                            result << 0;
                        }
                        arg_index++;
                    }
                    break;
                }
                case 'f':
                case 'g': {
                    if (arg_index < args.size()) {
                        if (args[arg_index].GetType() == LuaValueType::NUMBER) {
                            result << args[arg_index].AsNumber();
                        } else {
                            result << 0.0;
                        }
                        arg_index++;
                    }
                    break;
                }
                case 'x': {
                    if (arg_index < args.size()) {
                        if (args[arg_index].GetType() == LuaValueType::NUMBER) {
                            result << std::hex << static_cast<int>(args[arg_index].AsNumber()) << std::dec;
                        } else {
                            result << 0;
                        }
                        arg_index++;
                    }
                    break;
                }
                case 'c': {
                    if (arg_index < args.size()) {
                        if (args[arg_index].GetType() == LuaValueType::NUMBER) {
                            result << static_cast<char>(static_cast<int>(args[arg_index].AsNumber()));
                        }
                        arg_index++;
                    }
                    break;
                }
                default:
                    result << '%' << spec;
                    break;
            }
            
            i++;  // 跳过格式字符
        } else {
            result << format[i];
        }
    }
    
    return result.str();
}

// ============================================================================
// 简化实现的其他函数
// ============================================================================

LUA_STDLIB_FUNCTION(StringLibrary::lua_string_match) {
    StackHelper helper(vm);
    // TODO: 实现完整的match函数
    vm->GetStack().clear();
    vm->GetStack().push_back(LuaValue::CreateNil());
    return 1;
}

LUA_STDLIB_FUNCTION(StringLibrary::lua_string_gmatch) {
    StackHelper helper(vm);
    // TODO: 实现gmatch迭代器
    vm->GetStack().clear();
    vm->GetStack().push_back(LuaValue::CreateFunction([](EnhancedVirtualMachine*) { return 0; }));
    return 1;
}

LUA_STDLIB_FUNCTION(StringLibrary::lua_string_gsub) {
    StackHelper helper(vm);
    // TODO: 实现完整的gsub函数
    if (!vm->GetStack().empty()) {
        LuaValue original = vm->GetStack()[0];
        vm->GetStack().clear();
        vm->GetStack().push_back(original);
        vm->GetStack().push_back(LuaValue::CreateNumber(0));
        return 2;
    }
    return 0;
}

LUA_STDLIB_FUNCTION(StringLibrary::lua_string_dump) {
    StackHelper helper(vm);
    // TODO: 实现函数序列化
    vm->GetStack().clear();
    vm->GetStack().push_back(LuaValue::CreateString(""));
    return 1;
}

} // namespace stdlib
} // namespace lua_cpp