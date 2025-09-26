/**
 * @file stdlib_common.cpp
 * @brief T027 标准库通用实现
 * 
 * 实现标准库的通用功能和工具类
 * 
 * @author Lua C++ Project Team
 * @date 2025-09-26
 * @version 1.0
 */

#include "stdlib_common.h"
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <cstdarg>
#include <cctype>
#include <algorithm>

namespace lua_cpp {
namespace stdlib {

// ============================================================================
// StandardLibrary实现
// ============================================================================

StandardLibrary::StandardLibrary() = default;

StandardLibrary::~StandardLibrary() = default;

void StandardLibrary::RegisterModule(std::unique_ptr<LibraryModule> module) {
    if (!module) {
        throw std::invalid_argument("Cannot register null module");
    }
    
    std::string name = module->GetModuleName();
    if (modules_.find(name) != modules_.end()) {
        throw std::runtime_error("Module '" + name + "' already registered");
    }
    
    modules_[name] = std::move(module);
}

void StandardLibrary::InitializeAll(EnhancedVirtualMachine* vm) {
    if (!vm) {
        throw std::invalid_argument("VM cannot be null");
    }
    
    if (initialized_) {
        throw std::runtime_error("Standard library already initialized");
    }
    
    // 按依赖顺序初始化模块
    // 1. 基础库（最先初始化）
    if (auto base_lib = GetModule("base")) {
        base_lib->Initialize(vm);
        base_lib->RegisterModule(vm);
    }
    
    // 2. 其他核心库
    std::vector<std::string> core_modules = {"string", "table", "math"};
    for (const auto& name : core_modules) {
        if (auto module = GetModule(name)) {
            module->Initialize(vm);
            module->RegisterModule(vm);
        }
    }
    
    // 3. 可选库
    std::vector<std::string> optional_modules = {"io", "os", "debug", "package"};
    for (const auto& name : optional_modules) {
        if (auto module = GetModule(name)) {
            module->Initialize(vm);
            module->RegisterModule(vm);
        }
    }
    
    initialized_ = true;
}

LibraryModule* StandardLibrary::GetModule(const std::string& name) const {
    auto it = modules_.find(name);
    return (it != modules_.end()) ? it->second.get() : nullptr;
}

std::vector<std::string> StandardLibrary::GetModuleNames() const {
    std::vector<std::string> names;
    names.reserve(modules_.size());
    
    for (const auto& pair : modules_) {
        names.push_back(pair.first);
    }
    
    return names;
}

void StandardLibrary::CleanupAll(EnhancedVirtualMachine* vm) {
    if (!vm || !initialized_) {
        return;
    }
    
    // 按相反顺序清理
    for (auto it = modules_.rbegin(); it != modules_.rend(); ++it) {
        it->second->Cleanup(vm);
    }
    
    initialized_ = false;
}

// ============================================================================
// StackHelper实现
// ============================================================================

void StackHelper::CheckArgCount(int expected, const std::string& func_name) const {
    int actual = static_cast<int>(vm_->GetStack().size());
    if (actual != expected) {
        ErrorHelper::ArgError(func_name, -1, 
            "expected " + std::to_string(expected) + " arguments, got " + std::to_string(actual));
    }
}

void StackHelper::CheckArgRange(int min_args, int max_args, const std::string& func_name) const {
    int actual = static_cast<int>(vm_->GetStack().size());
    if (actual < min_args || actual > max_args) {
        ErrorHelper::ArgError(func_name, -1,
            "expected " + std::to_string(min_args) + "-" + std::to_string(max_args) + 
            " arguments, got " + std::to_string(actual));
    }
}

void StackHelper::CheckArgType(int index, LuaValueType expected_type, const std::string& func_name) const {
    const auto& stack = vm_->GetStack();
    if (index <= 0 || index > static_cast<int>(stack.size())) {
        ErrorHelper::ArgError(func_name, index, "invalid argument index");
    }
    
    const LuaValue& value = stack[index - 1];  // Lua使用1基索引
    if (value.GetType() != expected_type) {
        std::string actual_type = "unknown";
        switch (value.GetType()) {
            case LuaValueType::NIL: actual_type = "nil"; break;
            case LuaValueType::BOOLEAN: actual_type = "boolean"; break;
            case LuaValueType::NUMBER: actual_type = "number"; break;
            case LuaValueType::STRING: actual_type = "string"; break;
            case LuaValueType::TABLE: actual_type = "table"; break;
            case LuaValueType::FUNCTION: actual_type = "function"; break;
            case LuaValueType::USERDATA: actual_type = "userdata"; break;
            case LuaValueType::THREAD: actual_type = "thread"; break;
        }
        
        std::string expected_type_str = "unknown";
        switch (expected_type) {
            case LuaValueType::NIL: expected_type_str = "nil"; break;
            case LuaValueType::BOOLEAN: expected_type_str = "boolean"; break;
            case LuaValueType::NUMBER: expected_type_str = "number"; break;
            case LuaValueType::STRING: expected_type_str = "string"; break;
            case LuaValueType::TABLE: expected_type_str = "table"; break;
            case LuaValueType::FUNCTION: expected_type_str = "function"; break;
            case LuaValueType::USERDATA: expected_type_str = "userdata"; break;
            case LuaValueType::THREAD: expected_type_str = "thread"; break;
        }
        
        ErrorHelper::TypeError(func_name, index, expected_type_str, actual_type);
    }
}

std::string StackHelper::GetStringArg(int index, const std::string& default_value) const {
    const auto& stack = vm_->GetStack();
    if (index <= 0 || index > static_cast<int>(stack.size())) {
        return default_value;
    }
    
    const LuaValue& value = stack[index - 1];
    if (value.GetType() == LuaValueType::STRING) {
        return value.AsString();
    } else if (value.GetType() == LuaValueType::NUMBER) {
        // Lua会自动将数字转换为字符串
        std::ostringstream oss;
        oss << value.AsNumber();
        return oss.str();
    }
    
    return default_value;
}

double StackHelper::GetNumberArg(int index, double default_value) const {
    const auto& stack = vm_->GetStack();
    if (index <= 0 || index > static_cast<int>(stack.size())) {
        return default_value;
    }
    
    const LuaValue& value = stack[index - 1];
    if (value.GetType() == LuaValueType::NUMBER) {
        return value.AsNumber();
    } else if (value.GetType() == LuaValueType::STRING) {
        // 尝试将字符串转换为数字
        try {
            return std::stod(value.AsString());
        } catch (const std::exception&) {
            return default_value;
        }
    }
    
    return default_value;
}

int StackHelper::GetIntArg(int index, int default_value) const {
    double num = GetNumberArg(index, default_value);
    return static_cast<int>(num);
}

bool StackHelper::GetBoolArg(int index, bool default_value) const {
    const auto& stack = vm_->GetStack();
    if (index <= 0 || index > static_cast<int>(stack.size())) {
        return default_value;
    }
    
    const LuaValue& value = stack[index - 1];
    if (value.GetType() == LuaValueType::BOOLEAN) {
        return value.AsBoolean();
    } else if (value.GetType() == LuaValueType::NIL) {
        return false;  // nil被视为false
    }
    
    return true;  // 其他所有值都被视为true
}

// ============================================================================
// ErrorHelper实现
// ============================================================================

void ErrorHelper::ArgError(const std::string& func_name, int arg_index, const std::string& message) {
    std::ostringstream oss;
    oss << func_name << ": ";
    if (arg_index > 0) {
        oss << "bad argument #" << arg_index << " (" << message << ")";
    } else {
        oss << message;
    }
    throw std::runtime_error(oss.str());
}

void ErrorHelper::TypeError(const std::string& func_name, int arg_index, 
                          const std::string& expected_type, const std::string& actual_type) {
    std::ostringstream oss;
    oss << func_name << ": bad argument #" << arg_index 
        << " (" << expected_type << " expected, got " << actual_type << ")";
    throw std::runtime_error(oss.str());
}

void ErrorHelper::RuntimeError(const std::string& func_name, const std::string& message) {
    throw std::runtime_error(func_name + ": " + message);
}

// ============================================================================
// StringHelper实现
// ============================================================================

std::string StringHelper::Format(const char* format, const std::vector<LuaValue>& args) {
    // 简化的printf风格格式化实现
    // TODO: 实现完整的Lua string.format兼容性
    std::ostringstream result;
    const char* p = format;
    size_t arg_index = 0;
    
    while (*p) {
        if (*p == '%' && *(p + 1)) {
            p++;  // 跳过 %
            
            if (*p == '%') {
                result << '%';
            } else if (*p == 's') {
                if (arg_index < args.size()) {
                    if (args[arg_index].GetType() == LuaValueType::STRING) {
                        result << args[arg_index].AsString();
                    } else {
                        result << "[invalid string]";
                    }
                    arg_index++;
                }
            } else if (*p == 'd' || *p == 'i') {
                if (arg_index < args.size()) {
                    if (args[arg_index].GetType() == LuaValueType::NUMBER) {
                        result << static_cast<int>(args[arg_index].AsNumber());
                    } else {
                        result << 0;
                    }
                    arg_index++;
                }
            } else if (*p == 'f') {
                if (arg_index < args.size()) {
                    if (args[arg_index].GetType() == LuaValueType::NUMBER) {
                        result << args[arg_index].AsNumber();
                    } else {
                        result << 0.0;
                    }
                    arg_index++;
                }
            }
        } else {
            result << *p;
        }
        p++;
    }
    
    return result.str();
}

bool StringHelper::IsValidIdentifier(const std::string& str) {
    if (str.empty() || std::isdigit(str[0])) {
        return false;
    }
    
    return std::all_of(str.begin(), str.end(), [](char c) {
        return std::isalnum(c) || c == '_';
    });
}

std::string StringHelper::Escape(const std::string& str) {
    std::ostringstream result;
    
    for (char c : str) {
        switch (c) {
            case '\a': result << "\\a"; break;
            case '\b': result << "\\b"; break;
            case '\f': result << "\\f"; break;
            case '\n': result << "\\n"; break;
            case '\r': result << "\\r"; break;
            case '\t': result << "\\t"; break;
            case '\v': result << "\\v"; break;
            case '\\': result << "\\\\"; break;
            case '"': result << "\\\""; break;
            case '\'': result << "\\'"; break;
            default:
                if (std::isprint(c)) {
                    result << c;
                } else {
                    result << "\\x" << std::hex << std::setw(2) << std::setfill('0') 
                           << static_cast<unsigned char>(c);
                }
                break;
        }
    }
    
    return result.str();
}

std::string StringHelper::Unescape(const std::string& str) {
    std::ostringstream result;
    
    for (size_t i = 0; i < str.length(); i++) {
        if (str[i] == '\\' && i + 1 < str.length()) {
            char next = str[i + 1];
            switch (next) {
                case 'a': result << '\a'; break;
                case 'b': result << '\b'; break;
                case 'f': result << '\f'; break;
                case 'n': result << '\n'; break;
                case 'r': result << '\r'; break;
                case 't': result << '\t'; break;
                case 'v': result << '\v'; break;
                case '\\': result << '\\'; break;
                case '"': result << '"'; break;
                case '\'': result << '\''; break;
                default: 
                    result << str[i] << next;  // 保留未识别的转义序列
                    break;
            }
            i++;  // 跳过转义字符
        } else {
            result << str[i];
        }
    }
    
    return result.str();
}

} // namespace stdlib
} // namespace lua_cpp