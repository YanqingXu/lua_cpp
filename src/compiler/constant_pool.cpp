/**
 * @file constant_pool.cpp
 * @brief 常量池管理器实现
 * @description 管理编译时常量的存储和去重
 * @author Lua C++ Project
 * @date 2025-09-25
 */

#include "constant_pool.h"
#include "../core/lua_errors.h"
#include <algorithm>

namespace lua_cpp {

/* ========================================================================== */
/* ConstantPool 实现 */
/* ========================================================================== */

ConstantPool::ConstantPool() {
    // 预留一些空间以提高性能
    constants_.reserve(32);
    constant_map_.reserve(32);
}

ConstantPool::~ConstantPool() = default;

int ConstantPool::AddConstant(const LuaValue& value) {
    // 尝试查找现有常量
    auto it = constant_map_.find(value);
    if (it != constant_map_.end()) {
        return it->second;
    }
    
    // 检查常量池大小限制
    if (constants_.size() >= MAXARG_Bx) {
        throw CompilerError("Constant pool overflow: too many constants");
    }
    
    // 添加新常量
    int index = static_cast<int>(constants_.size());
    constants_.push_back(value);
    constant_map_[value] = index;
    
    return index;
}

int ConstantPool::FindConstant(const LuaValue& value) const {
    auto it = constant_map_.find(value);
    return (it != constant_map_.end()) ? it->second : -1;
}

const LuaValue& ConstantPool::GetConstant(int index) const {
    if (index < 0 || index >= static_cast<int>(constants_.size())) {
        throw CompilerError("Invalid constant index: " + std::to_string(index));
    }
    return constants_[index];
}

Size ConstantPool::GetSize() const {
    return constants_.size();
}

const std::vector<LuaValue>& ConstantPool::GetConstants() const {
    return constants_;
}

bool ConstantPool::IsEmpty() const {
    return constants_.empty();
}

void ConstantPool::Clear() {
    constants_.clear();
    constant_map_.clear();
}

void ConstantPool::Reserve(Size capacity) {
    constants_.reserve(capacity);
    constant_map_.reserve(capacity);
}

int ConstantPool::AddNumber(double number) {
    return AddConstant(LuaValue::CreateNumber(number));
}

int ConstantPool::AddString(const std::string& str) {
    return AddConstant(LuaValue::CreateString(str));
}

int ConstantPool::AddBoolean(bool value) {
    return AddConstant(LuaValue::CreateBool(value));
}

int ConstantPool::AddNil() {
    return AddConstant(LuaValue::CreateNil());
}

std::vector<int> ConstantPool::FindConstantsByType(LuaType type) const {
    std::vector<int> indices;
    for (Size i = 0; i < constants_.size(); ++i) {
        if (constants_[i].GetType() == type) {
            indices.push_back(static_cast<int>(i));
        }
    }
    return indices;
}

bool ConstantPool::HasConstant(const LuaValue& value) const {
    return constant_map_.find(value) != constant_map_.end();
}

void ConstantPool::OptimizeStorage() {
    // 移除重复常量（虽然理论上不应该有重复）
    // 这个函数主要是为了紧缩存储，在当前实现中可能不需要做什么
    
    // 可以在这里实现一些优化，比如：
    // 1. 重新排序常量以提高缓存局部性
    // 2. 压缩相似的字符串常量
    // 3. 合并相近的数值常量等
}

std::string ConstantPool::ToString() const {
    std::string result = "Constant Pool (" + std::to_string(constants_.size()) + " entries):\n";
    
    for (Size i = 0; i < constants_.size(); ++i) {
        result += "  [" + std::to_string(i) + "]: ";
        
        const LuaValue& value = constants_[i];
        switch (value.GetType()) {
            case LuaType::Nil:
                result += "nil";
                break;
            case LuaType::Bool:
                result += value.AsBool() ? "true" : "false";
                break;
            case LuaType::Number:
                result += std::to_string(value.AsNumber());
                break;
            case LuaType::String:
                result += "\"" + value.AsString() + "\"";
                break;
            default:
                result += "<unknown>";
                break;
        }
        
        result += "\n";
    }
    
    return result;
}

/* ========================================================================== */
/* ConstantPoolBuilder 实现 */
/* ========================================================================== */

ConstantPoolBuilder::ConstantPoolBuilder() : pool_() {
}

ConstantPoolBuilder::~ConstantPoolBuilder() = default;

int ConstantPoolBuilder::AddConstant(const LuaValue& value) {
    return pool_.AddConstant(value);
}

int ConstantPoolBuilder::AddNumber(double number) {
    return pool_.AddNumber(number);
}

int ConstantPoolBuilder::AddString(const std::string& str) {
    return pool_.AddString(str);
}

int ConstantPoolBuilder::AddBoolean(bool value) {
    return pool_.AddBoolean(value);
}

int ConstantPoolBuilder::AddNil() {
    return pool_.AddNil();
}

int ConstantPoolBuilder::FindOrAddConstant(const LuaValue& value) {
    int existing = pool_.FindConstant(value);
    return (existing >= 0) ? existing : pool_.AddConstant(value);
}

int ConstantPoolBuilder::FindOrAddNumber(double number) {
    return FindOrAddConstant(LuaValue::CreateNumber(number));
}

int ConstantPoolBuilder::FindOrAddString(const std::string& str) {
    return FindOrAddConstant(LuaValue::CreateString(str));
}

int ConstantPoolBuilder::FindOrAddBoolean(bool value) {
    return FindOrAddConstant(LuaValue::CreateBool(value));
}

ConstantPool ConstantPoolBuilder::Build() && {
    return std::move(pool_);
}

const ConstantPool& ConstantPoolBuilder::GetPool() const {
    return pool_;
}

Size ConstantPoolBuilder::GetSize() const {
    return pool_.GetSize();
}

void ConstantPoolBuilder::Clear() {
    pool_.Clear();
}

void ConstantPoolBuilder::Reserve(Size capacity) {
    pool_.Reserve(capacity);
}

/* ========================================================================== */
/* 常量优化函数 */
/* ========================================================================== */

bool CanBeInlined(const LuaValue& value) {
    // 某些常量可以直接内联到指令中
    switch (value.GetType()) {
        case LuaType::Nil:
            return true; // nil 可以用 LOADNIL 指令
        case LuaType::Bool:
            return true; // 布尔值可以用 LOADBOOL 指令
        case LuaType::Number: {
            double num = value.AsNumber();
            // 小整数可以考虑内联
            return (num == static_cast<int>(num) && num >= -128 && num <= 127);
        }
        case LuaType::String: {
            // 空字符串可以考虑特殊处理
            return value.AsString().empty();
        }
        default:
            return false;
    }
}

LuaValue FoldConstants(const LuaValue& left, const LuaValue& right, OpCode op) {
    // 常量折叠：在编译时计算常量表达式的值
    if (left.GetType() != LuaType::Number || right.GetType() != LuaType::Number) {
        return LuaValue::CreateNil(); // 无法折叠
    }
    
    double l = left.AsNumber();
    double r = right.AsNumber();
    
    switch (op) {
        case OpCode::ADD:
            return LuaValue::CreateNumber(l + r);
        case OpCode::SUB:
            return LuaValue::CreateNumber(l - r);
        case OpCode::MUL:
            return LuaValue::CreateNumber(l * r);
        case OpCode::DIV:
            if (r != 0.0) {
                return LuaValue::CreateNumber(l / r);
            }
            break;
        case OpCode::MOD:
            if (r != 0.0) {
                return LuaValue::CreateNumber(std::fmod(l, r));
            }
            break;
        case OpCode::POW:
            return LuaValue::CreateNumber(std::pow(l, r));
        default:
            break;
    }
    
    return LuaValue::CreateNil(); // 无法折叠或出错
}

LuaValue FoldUnaryConstant(const LuaValue& operand, OpCode op) {
    switch (op) {
        case OpCode::UNM:
            if (operand.GetType() == LuaType::Number) {
                return LuaValue::CreateNumber(-operand.AsNumber());
            }
            break;
        case OpCode::NOT:
            // Lua 中只有 false 和 nil 为假
            if (operand.GetType() == LuaType::Nil) {
                return LuaValue::CreateBool(true);
            } else if (operand.GetType() == LuaType::Bool) {
                return LuaValue::CreateBool(!operand.AsBool());
            } else {
                return LuaValue::CreateBool(false); // 其他值都为真
            }
            break;
        case OpCode::LEN:
            if (operand.GetType() == LuaType::String) {
                return LuaValue::CreateNumber(static_cast<double>(operand.AsString().length()));
            }
            break;
        default:
            break;
    }
    
    return LuaValue::CreateNil(); // 无法折叠
}

bool IsConstantExpression(const LuaValue& value) {
    // 检查值是否为编译时常量
    switch (value.GetType()) {
        case LuaType::Nil:
        case LuaType::Bool:
        case LuaType::Number:
        case LuaType::String:
            return true;
        default:
            return false;
    }
}

Size EstimateConstantPoolSize(const std::vector<LuaValue>& values) {
    // 估算去重后的常量池大小
    std::unordered_set<LuaValue, LuaValueHash> unique_values;
    for (const auto& value : values) {
        unique_values.insert(value);
    }
    return unique_values.size();
}

void OptimizeConstantPool(ConstantPool& pool) {
    // 优化常量池
    pool.OptimizeStorage();
}

} // namespace lua_cpp