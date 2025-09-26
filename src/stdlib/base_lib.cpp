/**
 * @file base_lib.cpp
 * @brief T027 Lua基础库实现
 * 
 * 实现Lua 5.1.5基础库的所有核心函数
 * 
 * @author Lua C++ Project Team
 * @date 2025-09-26
 * @version 1.0
 */

#include "base_lib.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <cstring>
#include <cstdlib>
#include <algorithm>

namespace lua_cpp {
namespace stdlib {

// ============================================================================
// BaseLibrary类实现
// ============================================================================

BaseLibrary::BaseLibrary() = default;

std::vector<LibFunction> BaseLibrary::GetFunctions() const {
    std::vector<LibFunction> functions;
    
    // 类型检查和操作
    REGISTER_FUNCTION(functions, "type", lua_type, "返回给定值的类型");
    REGISTER_FUNCTION(functions, "getmetatable", lua_getmetatable, "获取对象的元表");
    REGISTER_FUNCTION(functions, "setmetatable", lua_setmetatable, "设置表的元表");
    
    // 类型转换
    REGISTER_FUNCTION(functions, "tostring", lua_tostring, "将值转换为字符串");
    REGISTER_FUNCTION(functions, "tonumber", lua_tonumber, "将字符串转换为数字");
    
    // 表操作
    REGISTER_FUNCTION(functions, "rawget", lua_rawget, "原始表访问");
    REGISTER_FUNCTION(functions, "rawset", lua_rawset, "原始表设置");
    REGISTER_FUNCTION(functions, "rawequal", lua_rawequal, "原始相等比较");
    REGISTER_FUNCTION(functions, "rawlen", lua_rawlen, "原始长度操作");
    
    // 迭代器
    REGISTER_FUNCTION(functions, "next", lua_next, "表的下一个键值对");
    REGISTER_FUNCTION(functions, "pairs", lua_pairs, "表的键值对迭代器");
    REGISTER_FUNCTION(functions, "ipairs", lua_ipairs, "数组的索引迭代器");
    
    // 全局环境
    REGISTER_FUNCTION(functions, "getfenv", lua_getfenv, "获取函数环境");
    REGISTER_FUNCTION(functions, "setfenv", lua_setfenv, "设置函数环境");
    
    // 错误处理
    REGISTER_FUNCTION(functions, "error", lua_error, "抛出错误");
    REGISTER_FUNCTION(functions, "assert", lua_assert, "断言检查");
    REGISTER_FUNCTION(functions, "pcall", lua_pcall, "保护调用");
    REGISTER_FUNCTION(functions, "xpcall", lua_xpcall, "扩展保护调用");
    
    // 输出函数
    REGISTER_FUNCTION(functions, "print", lua_print, "打印输出");
    
    // 其他工具函数
    REGISTER_FUNCTION(functions, "select", lua_select, "选择参数");
    REGISTER_FUNCTION(functions, "unpack", lua_unpack, "解包数组");
    REGISTER_FUNCTION(functions, "loadstring", lua_loadstring, "加载字符串");
    REGISTER_FUNCTION(functions, "loadfile", lua_loadfile, "加载文件");
    REGISTER_FUNCTION(functions, "dofile", lua_dofile, "执行文件");
    REGISTER_FUNCTION(functions, "collectgarbage", lua_collectgarbage, "垃圾回收");
    
    return functions;
}

void BaseLibrary::RegisterModule(EnhancedVirtualMachine* vm) {
    if (!vm) return;
    
    auto functions = GetFunctions();
    
    // 将基础函数注册到全局环境
    auto& globals = vm->GetGlobalEnvironment();
    
    for (const auto& func : functions) {
        // 基础库函数直接注册到全局命名空间
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
        
        globals.SetField(func.name, func_value);
    }
}

void BaseLibrary::Initialize(EnhancedVirtualMachine* vm) {
    // 基础库初始化逻辑
    if (!vm) return;
    
    // 设置全局变量 _G 指向全局环境
    auto& globals = vm->GetGlobalEnvironment();
    globals.SetField("_G", LuaValue::CreateTable(globals));
    
    // 设置版本信息
    globals.SetField("_VERSION", LuaValue::CreateString("Lua 5.1.5 (lua_cpp)"));
}

void BaseLibrary::Cleanup(EnhancedVirtualMachine* vm) {
    // 基础库清理逻辑
    (void)vm;  // 暂时不需要特殊清理
}

// ============================================================================
// 基础库函数实现
// ============================================================================

LUA_STDLIB_FUNCTION(BaseLibrary::lua_type) {
    StackHelper helper(vm);
    helper.CheckArgCount(1, "type");
    
    const LuaValue& value = vm->GetStack()[0];
    std::string type_name = GetTypeName(value);
    
    // 清理参数
    vm->GetStack().clear();
    
    // 压入结果
    vm->GetStack().push_back(LuaValue::CreateString(type_name));
    
    return 1;
}

LUA_STDLIB_FUNCTION(BaseLibrary::lua_tostring) {
    StackHelper helper(vm);
    helper.CheckArgCount(1, "tostring");
    
    const LuaValue& value = vm->GetStack()[0];
    std::string str_value = ValueToString(value);
    
    // 清理参数
    vm->GetStack().clear();
    
    // 压入结果
    vm->GetStack().push_back(LuaValue::CreateString(str_value));
    
    return 1;
}

LUA_STDLIB_FUNCTION(BaseLibrary::lua_tonumber) {
    StackHelper helper(vm);
    
    if (vm->GetStack().size() < 1 || vm->GetStack().size() > 2) {
        ErrorHelper::ArgError("tonumber", -1, "expected 1 or 2 arguments");
    }
    
    const LuaValue& value = vm->GetStack()[0];
    int base = 10;
    
    if (vm->GetStack().size() == 2) {
        base = helper.GetIntArg(2, 10);
        if (base < 2 || base > 36) {
            ErrorHelper::ArgError("tonumber", 2, "base out of range");
        }
    }
    
    double result;
    bool success = false;
    
    if (value.GetType() == LuaValueType::NUMBER) {
        result = value.AsNumber();
        success = (base == 10);  // 只有10进制时数字才有效
    } else if (value.GetType() == LuaValueType::STRING) {
        success = StringToNumber(value.AsString(), result, base);
    }
    
    // 清理参数
    vm->GetStack().clear();
    
    // 压入结果
    if (success) {
        vm->GetStack().push_back(LuaValue::CreateNumber(result));
    } else {
        vm->GetStack().push_back(LuaValue::CreateNil());
    }
    
    return 1;
}

LUA_STDLIB_FUNCTION(BaseLibrary::lua_rawget) {
    StackHelper helper(vm);
    helper.CheckArgCount(2, "rawget");
    helper.CheckArgType(1, LuaValueType::TABLE, "rawget");
    
    const LuaTable& table = vm->GetStack()[0].AsTable();
    const LuaValue& key = vm->GetStack()[1];
    
    LuaValue result = table.GetField(key);
    
    // 清理参数
    vm->GetStack().clear();
    
    // 压入结果
    vm->GetStack().push_back(result);
    
    return 1;
}

LUA_STDLIB_FUNCTION(BaseLibrary::lua_rawset) {
    StackHelper helper(vm);
    helper.CheckArgCount(3, "rawset");
    helper.CheckArgType(1, LuaValueType::TABLE, "rawset");
    
    LuaTable& table = const_cast<LuaTable&>(vm->GetStack()[0].AsTable());
    const LuaValue& key = vm->GetStack()[1];
    const LuaValue& value = vm->GetStack()[2];
    
    if (key.GetType() == LuaValueType::NIL) {
        ErrorHelper::ArgError("rawset", 2, "table index is nil");
    }
    
    table.SetField(key, value);
    
    // 清理参数，保留表作为返回值
    LuaValue table_copy = vm->GetStack()[0];
    vm->GetStack().clear();
    vm->GetStack().push_back(table_copy);
    
    return 1;
}

LUA_STDLIB_FUNCTION(BaseLibrary::lua_rawequal) {
    StackHelper helper(vm);
    helper.CheckArgCount(2, "rawequal");
    
    const LuaValue& v1 = vm->GetStack()[0];
    const LuaValue& v2 = vm->GetStack()[1];
    
    bool equal = v1.RawEqual(v2);
    
    // 清理参数
    vm->GetStack().clear();
    
    // 压入结果
    vm->GetStack().push_back(LuaValue::CreateBoolean(equal));
    
    return 1;
}

LUA_STDLIB_FUNCTION(BaseLibrary::lua_rawlen) {
    StackHelper helper(vm);
    helper.CheckArgCount(1, "rawlen");
    
    const LuaValue& value = vm->GetStack()[0];
    size_t length = 0;
    
    switch (value.GetType()) {
        case LuaValueType::STRING:
            length = value.AsString().length();
            break;
        case LuaValueType::TABLE:
            length = GetSequenceLength(value.AsTable());
            break;
        default:
            ErrorHelper::ArgError("rawlen", 1, "object has no length");
    }
    
    // 清理参数
    vm->GetStack().clear();
    
    // 压入结果
    vm->GetStack().push_back(LuaValue::CreateNumber(static_cast<double>(length)));
    
    return 1;
}

LUA_STDLIB_FUNCTION(BaseLibrary::lua_print) {
    auto& stack = vm->GetStack();
    
    // 收集所有参数并转换为字符串
    std::vector<std::string> args;
    args.reserve(stack.size());
    
    for (const auto& arg : stack) {
        args.push_back(ValueToString(arg));
    }
    
    // 清理参数
    stack.clear();
    
    // 打印输出（用制表符分隔）
    if (!args.empty()) {
        std::cout << args[0];
        for (size_t i = 1; i < args.size(); i++) {
            std::cout << "\t" << args[i];
        }
    }
    std::cout << std::endl;
    
    return 0;  // print不返回任何值
}

LUA_STDLIB_FUNCTION(BaseLibrary::lua_next) {
    StackHelper helper(vm);
    
    if (vm->GetStack().size() < 1 || vm->GetStack().size() > 2) {
        ErrorHelper::ArgError("next", -1, "expected 1 or 2 arguments");
    }
    
    helper.CheckArgType(1, LuaValueType::TABLE, "next");
    
    const LuaTable& table = vm->GetStack()[0].AsTable();
    LuaValue key = (vm->GetStack().size() == 2) ? vm->GetStack()[1] : LuaValue::CreateNil();
    
    // 获取下一个键值对
    auto next_pair = table.GetNextPair(key);
    
    // 清理参数
    vm->GetStack().clear();
    
    // 压入结果
    if (next_pair.first.GetType() != LuaValueType::NIL) {
        vm->GetStack().push_back(next_pair.first);   // key
        vm->GetStack().push_back(next_pair.second);  // value
        return 2;
    } else {
        return 0;  // 没有更多元素
    }
}

// ============================================================================
// 内部辅助函数实现
// ============================================================================

std::string BaseLibrary::GetTypeName(const LuaValue& value) {
    switch (value.GetType()) {
        case LuaValueType::NIL: return "nil";
        case LuaValueType::BOOLEAN: return "boolean";
        case LuaValueType::NUMBER: return "number";
        case LuaValueType::STRING: return "string";
        case LuaValueType::TABLE: return "table";
        case LuaValueType::FUNCTION: return "function";
        case LuaValueType::USERDATA: return "userdata";
        case LuaValueType::THREAD: return "thread";
        default: return "unknown";
    }
}

std::string BaseLibrary::ValueToString(const LuaValue& value) {
    switch (value.GetType()) {
        case LuaValueType::NIL:
            return "nil";
        case LuaValueType::BOOLEAN:
            return value.AsBoolean() ? "true" : "false";
        case LuaValueType::NUMBER: {
            double num = value.AsNumber();
            // 检查是否为整数
            if (num == std::floor(num) && std::isfinite(num)) {
                return std::to_string(static_cast<long long>(num));
            } else {
                std::ostringstream oss;
                oss << num;
                return oss.str();
            }
        }
        case LuaValueType::STRING:
            return value.AsString();
        case LuaValueType::TABLE:
            return "table: " + std::to_string(reinterpret_cast<uintptr_t>(&value.AsTable()));
        case LuaValueType::FUNCTION:
            return "function: " + std::to_string(reinterpret_cast<uintptr_t>(&value.AsFunction()));
        case LuaValueType::USERDATA:
            return "userdata: " + std::to_string(reinterpret_cast<uintptr_t>(value.AsUserdata()));
        case LuaValueType::THREAD:
            return "thread: " + std::to_string(reinterpret_cast<uintptr_t>(&value.AsThread()));
        default:
            return "unknown";
    }
}

bool BaseLibrary::StringToNumber(const std::string& str, double& result, int base) {
    if (str.empty()) {
        return false;
    }
    
    // 移除前后空格
    std::string trimmed = str;
    trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r\f\v"));
    trimmed.erase(trimmed.find_last_not_of(" \t\n\r\f\v") + 1);
    
    if (trimmed.empty()) {
        return false;
    }
    
    try {
        if (base == 10) {
            // 十进制，支持浮点数
            size_t pos;
            result = std::stod(trimmed, &pos);
            return pos == trimmed.length();  // 确保整个字符串都被转换
        } else {
            // 其他进制，只支持整数
            if (trimmed.find('.') != std::string::npos) {
                return false;  // 非十进制不支持小数点
            }
            
            char* endptr;
            long long int_result = std::strtoll(trimmed.c_str(), &endptr, base);
            
            if (endptr == trimmed.c_str() || *endptr != '\0') {
                return false;  // 转换失败或有剩余字符
            }
            
            result = static_cast<double>(int_result);
            return true;
        }
    } catch (const std::exception&) {
        return false;
    }
}

size_t BaseLibrary::GetSequenceLength(const LuaTable& table) {
    // 计算表的序列长度（连续的整数索引）
    size_t length = 0;
    
    for (size_t i = 1; ; i++) {
        LuaValue key = LuaValue::CreateNumber(static_cast<double>(i));
        LuaValue value = table.GetField(key);
        
        if (value.GetType() == LuaValueType::NIL) {
            break;
        }
        
        length = i;
    }
    
    return length;
}

// ============================================================================
// 其他基础函数实现（简化版本）
// ============================================================================

LUA_STDLIB_FUNCTION(BaseLibrary::lua_pairs) {
    // 简化实现：返回next, table, nil
    StackHelper helper(vm);
    helper.CheckArgCount(1, "pairs");
    helper.CheckArgType(1, LuaValueType::TABLE, "pairs");
    
    LuaValue table = vm->GetStack()[0];
    vm->GetStack().clear();
    
    // TODO: 实现完整的pairs迭代器
    // 暂时返回表示成功
    vm->GetStack().push_back(LuaValue::CreateFunction([](EnhancedVirtualMachine*) { return 0; }));
    vm->GetStack().push_back(table);
    vm->GetStack().push_back(LuaValue::CreateNil());
    
    return 3;
}

LUA_STDLIB_FUNCTION(BaseLibrary::lua_ipairs) {
    // 简化实现：返回ipairs_iterator, table, 0
    StackHelper helper(vm);
    helper.CheckArgCount(1, "ipairs");
    helper.CheckArgType(1, LuaValueType::TABLE, "ipairs");
    
    LuaValue table = vm->GetStack()[0];
    vm->GetStack().clear();
    
    // TODO: 实现完整的ipairs迭代器
    // 暂时返回表示成功
    vm->GetStack().push_back(LuaValue::CreateFunction([](EnhancedVirtualMachine*) { return 0; }));
    vm->GetStack().push_back(table);
    vm->GetStack().push_back(LuaValue::CreateNumber(0));
    
    return 3;
}

// 其他函数的简化实现
LUA_STDLIB_FUNCTION(BaseLibrary::lua_getmetatable) {
    StackHelper helper(vm);
    helper.CheckArgCount(1, "getmetatable");
    
    // TODO: 实现元表获取
    vm->GetStack().clear();
    vm->GetStack().push_back(LuaValue::CreateNil());
    return 1;
}

LUA_STDLIB_FUNCTION(BaseLibrary::lua_setmetatable) {
    StackHelper helper(vm);
    helper.CheckArgCount(2, "setmetatable");
    
    LuaValue table = vm->GetStack()[0];
    vm->GetStack().clear();
    vm->GetStack().push_back(table);
    return 1;
}

LUA_STDLIB_FUNCTION(BaseLibrary::lua_error) {
    StackHelper helper(vm);
    helper.CheckArgCount(1, "error");
    
    std::string message = helper.GetStringArg(1, "error");
    ErrorHelper::RuntimeError("error", message);
    return 0;
}

LUA_STDLIB_FUNCTION(BaseLibrary::lua_assert) {
    StackHelper helper(vm);
    
    if (vm->GetStack().empty()) {
        ErrorHelper::ArgError("assert", 1, "value expected");
    }
    
    const LuaValue& value = vm->GetStack()[0];
    
    // 在Lua中，只有nil和false被视为假值
    bool is_true = !(value.GetType() == LuaValueType::NIL || 
                    (value.GetType() == LuaValueType::BOOLEAN && !value.AsBoolean()));
    
    if (!is_true) {
        std::string message = "assertion failed";
        if (vm->GetStack().size() > 1) {
            message = helper.GetStringArg(2, message);
        }
        ErrorHelper::RuntimeError("assert", message);
    }
    
    // assert返回所有参数
    return static_cast<int>(vm->GetStack().size());
}

// 简化实现的其他函数
LUA_STDLIB_FUNCTION(BaseLibrary::lua_pcall) { vm->GetStack().clear(); return 0; }
LUA_STDLIB_FUNCTION(BaseLibrary::lua_xpcall) { vm->GetStack().clear(); return 0; }
LUA_STDLIB_FUNCTION(BaseLibrary::lua_getfenv) { vm->GetStack().clear(); return 0; }
LUA_STDLIB_FUNCTION(BaseLibrary::lua_setfenv) { vm->GetStack().clear(); return 0; }
LUA_STDLIB_FUNCTION(BaseLibrary::lua_require) { vm->GetStack().clear(); return 0; }
LUA_STDLIB_FUNCTION(BaseLibrary::lua_module) { vm->GetStack().clear(); return 0; }
LUA_STDLIB_FUNCTION(BaseLibrary::lua_select) { vm->GetStack().clear(); return 0; }
LUA_STDLIB_FUNCTION(BaseLibrary::lua_unpack) { vm->GetStack().clear(); return 0; }
LUA_STDLIB_FUNCTION(BaseLibrary::lua_loadstring) { vm->GetStack().clear(); return 0; }
LUA_STDLIB_FUNCTION(BaseLibrary::lua_loadfile) { vm->GetStack().clear(); return 0; }
LUA_STDLIB_FUNCTION(BaseLibrary::lua_dofile) { vm->GetStack().clear(); return 0; }
LUA_STDLIB_FUNCTION(BaseLibrary::lua_collectgarbage) { vm->GetStack().clear(); return 0; }

} // namespace stdlib
} // namespace lua_cpp