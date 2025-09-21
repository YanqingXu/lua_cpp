/**
 * @file value.h
 * @brief Lua值类型定义
 * @description 定义Lua中的所有值类型
 * @author Lua C++ Project
 * @date 2025-09-21
 */

#pragma once

#include "../core/lua_common.h"
#include <string>
#include <memory>
#include <variant>

namespace lua_cpp {

/* ========================================================================== */
/* 前向声明 */
/* ========================================================================== */

class LuaTable;
class LuaFunction;
class LuaUserdata;

/* ========================================================================== */
/* Lua值类型枚举 */
/* ========================================================================== */

// LuaType已在lua_common.h中定义，此处不重复定义

/* ========================================================================== */
/* Lua值类 */
/* ========================================================================== */

/**
 * @brief Lua值类
 * @description 表示Lua中的任何值
 */
class LuaValue {
public:
    /**
     * @brief 默认构造函数（创建nil值）
     */
    LuaValue() : type_(LuaType::Nil) {}
    
    /**
     * @brief 布尔值构造函数
     */
    explicit LuaValue(bool value) : type_(LuaType::Boolean), boolean_(value) {}
    
    /**
     * @brief 数值构造函数
     */
    explicit LuaValue(double value) : type_(LuaType::Number), number_(value) {}
    
    /**
     * @brief 整数构造函数
     */
    explicit LuaValue(int value) : type_(LuaType::Number), number_(static_cast<double>(value)) {}
    
    /**
     * @brief 字符串构造函数
     */
    explicit LuaValue(const std::string& value) : type_(LuaType::String), string_(value) {}
    
    /**
     * @brief 字符串构造函数
     */
    explicit LuaValue(const char* value) : type_(LuaType::String), string_(value) {}
    
    // 拷贝构造和赋值
    LuaValue(const LuaValue& other) = default;
    LuaValue& operator=(const LuaValue& other) = default;
    
    // 移动构造和赋值
    LuaValue(LuaValue&& other) noexcept = default;
    LuaValue& operator=(LuaValue&& other) noexcept = default;
    
    /**
     * @brief 析构函数
     */
    ~LuaValue() = default;
    
    /* ===== 类型检查方法 ===== */
    
    /**
     * @brief 获取值类型
     */
    LuaType GetType() const { return type_; }
    
    /**
     * @brief 检查是否为nil
     */
    bool IsNil() const { return type_ == LuaType::Nil; }
    
    /**
     * @brief 检查是否为布尔值
     */
    bool IsBoolean() const { return type_ == LuaType::Boolean; }
    
    /**
     * @brief 检查是否为数值
     */
    bool IsNumber() const { return type_ == LuaType::Number; }
    
    /**
     * @brief 检查是否为字符串
     */
    bool IsString() const { return type_ == LuaType::String; }
    
    /**
     * @brief 检查是否为表
     */
    bool IsTable() const { return type_ == LuaType::Table; }
    
    /**
     * @brief 检查是否为函数
     */
    bool IsFunction() const { return type_ == LuaType::Function; }
    
    /**
     * @brief 检查是否为用户数据
     */
    bool IsUserdata() const { return type_ == LuaType::Userdata; }
    
    /* ===== 值获取方法 ===== */
    
    /**
     * @brief 获取布尔值
     */
    bool AsBoolean() const {
        if (type_ == LuaType::Boolean) return boolean_;
        return IsTruthy();
    }
    
    /**
     * @brief 获取数值
     */
    double AsNumber() const {
        if (type_ == LuaType::Number) return number_;
        throw std::runtime_error("Value is not a number");
    }
    
    /**
     * @brief 获取字符串
     */
    const std::string& AsString() const {
        if (type_ == LuaType::String) return string_;
        throw std::runtime_error("Value is not a string");
    }
    
    /* ===== Lua真值性检查 ===== */
    
    /**
     * @brief 检查值的真值性（Lua语义）
     */
    bool IsTruthy() const {
        switch (type_) {
            case LuaType::Nil:
                return false;
            case LuaType::Boolean:
                return boolean_;
            default:
                return true; // 所有其他值都为真
        }
    }
    
    /* ===== 比较操作 ===== */
    
    /**
     * @brief 相等比较
     */
    bool operator==(const LuaValue& other) const {
        if (type_ != other.type_) return false;
        
        switch (type_) {
            case LuaType::Nil:
                return true;
            case LuaType::Boolean:
                return boolean_ == other.boolean_;
            case LuaType::Number:
                return number_ == other.number_;
            case LuaType::String:
                return string_ == other.string_;
            default:
                return false; // 复杂类型需要额外处理
        }
    }
    
    /**
     * @brief 不等比较
     */
    bool operator!=(const LuaValue& other) const {
        return !(*this == other);
    }
    
    /* ===== 转换方法 ===== */
    
    /**
     * @brief 转换为字符串表示
     */
    std::string ToString() const {
        switch (type_) {
            case LuaType::Nil:
                return "nil";
            case LuaType::Boolean:
                return boolean_ ? "true" : "false";
            case LuaType::Number:
                return std::to_string(number_);
            case LuaType::String:
                return string_;
            default:
                return "<" + TypeName() + ">";
        }
    }
    
    /**
     * @brief 获取类型名称
     */
    std::string TypeName() const {
        switch (type_) {
            case LuaType::Nil: return "nil";
            case LuaType::Boolean: return "boolean";
            case LuaType::Number: return "number";
            case LuaType::String: return "string";
            case LuaType::Table: return "table";
            case LuaType::Function: return "function";
            case LuaType::Userdata: return "userdata";
            case LuaType::Thread: return "thread";
            default: return "unknown";
        }
    }

private:
    LuaType type_;
    
    union {
        bool boolean_;
        double number_;
    };
    
    std::string string_;
    
    // TODO: 添加对复杂类型（table、function等）的支持
    // std::shared_ptr<LuaTable> table_;
    // std::shared_ptr<LuaFunction> function_;
    // std::shared_ptr<LuaUserdata> userdata_;
};

} // namespace lua_cpp