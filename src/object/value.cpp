#include "value.hpp"
#include "string.hpp"
#include "table.hpp"
#include "function.hpp"
#include "userdata.hpp"
#include "gc/garbage_collector.hpp"

#include <sstream>
#include <cmath>

namespace Lua {
namespace Object {

// 构造函数实现
Value::Value(const Str& value) {
    // 需要通过VM::State获取GarbageCollector实例
    // 由于这里无法直接访问state，暂时创建一个简单的字符串值
	m_value = make_ptr<String>(value);
}

Value::Value(Ptr<String> value) {
    m_value = value;
}

Value::Value(Ptr<Table> value) {
    m_value = value;
}

Value::Value(Ptr<Function> value) {
    m_value = value;
}

Value::Value(Ptr<UserData> value) {
    m_value = value;
}

// 静态工厂方法实现
Value Value::nil() {
    return Value(nullptr);
}

Value Value::boolean(bool value) {
    return Value(value);
}

Value Value::number(f64 value) {
    return Value(value);
}

Value Value::string(const Str& value) {
    return Value(value);
}

Value Value::string(Ptr<String> value) {
    return Value(value);
}

Value Value::table(Ptr<Table> value) {
    return Value(value);
}

Value Value::function(Ptr<Function> value) {
    return Value(value);
}

Value Value::userdata(Ptr<UserData> value) {
    return Value(value);
}

// 类型判断方法
bool Value::isNil() const {
    return std::holds_alternative<std::nullptr_t>(m_value);
}

bool Value::isBoolean() const {
    return std::holds_alternative<bool>(m_value);
}

bool Value::isNumber() const {
    return std::holds_alternative<f64>(m_value);
}

bool Value::isGCObject() const {
    return std::holds_alternative<Ptr<GC::GCObject>>(m_value);
}

bool Value::isString() const {
    return std::holds_alternative<Str>(m_value);
}

bool Value::isTable() const {
    if (!isGCObject()) {
        return false;
    }
    
    auto obj = std::get<Ptr<GC::GCObject>>(m_value);
    return obj && obj->type() == GC::GCObject::Type::Table;
}

bool Value::isFunction() const {
    if (!isGCObject()) {
        return false;
    }
    
    auto obj = std::get<Ptr<GC::GCObject>>(m_value);
    return obj && obj->type() == GC::GCObject::Type::Function;
}

bool Value::isUserData() const {
    if (!isGCObject()) {
        return false;
    }
    
    auto obj = std::get<Ptr<GC::GCObject>>(m_value);
    return obj && obj->type() == GC::GCObject::Type::UserData;
}

// 类型转换方法
bool Value::asBoolean() const {
    if (isBoolean()) {
        return std::get<bool>(m_value);
    }
    // Lua真值规则：nil和false为假，其他都为真
    return !isNil();
}

f64 Value::asNumber() const {
    if (isNumber()) {
        return std::get<f64>(m_value);
    }
    
    // 尝试从字符串转换为数字
    if (isString()) {
        try {
            return std::stod(asString());
        } catch (...) {
            // 无法转换
        }
    }
    
    throw std::runtime_error("Value is not a number");
}

Str Value::asString() const {
    if (isString()) {
        return std::get<Str>(m_value);
    }
    
    // 数字转字符串
    if (isNumber()) {
        std::ostringstream oss;
        f64 num = asNumber();
        
        if (std::floor(num) == num && std::abs(num) < 1e10) {
            // 整数表示
            oss << static_cast<i64>(num);
        } else {
            // 浮点数表示
            oss << num;
        }
        
        return oss.str();
    }
    
    throw std::runtime_error("Value cannot be converted to string");
}

Ptr<String> Value::asStringObject() const {
    // 当前实现中，字符串不再以String对象形式存储
    throw std::runtime_error("String objects not implemented");
}

Ptr<Table> Value::asTable() const {
    if (!isTable()) {
        throw std::runtime_error("Value is not a table");
    }
    
    return std::static_pointer_cast<Table>(std::get<Ptr<GC::GCObject>>(m_value));
}

Ptr<Function> Value::asFunction() const {
    if (!isFunction()) {
        throw std::runtime_error("Value is not a function");
    }
    
    return std::static_pointer_cast<Function>(std::get<Ptr<GC::GCObject>>(m_value));
}

Ptr<UserData> Value::asUserData() const {
    if (!isUserData()) {
        throw std::runtime_error("Value is not a userdata");
    }
    
    return std::static_pointer_cast<UserData>(std::get<Ptr<GC::GCObject>>(m_value));
}

Ptr<GC::GCObject> Value::asGCObject() const {
    if (!isGCObject()) {
        return nullptr;
    }
    
    return std::get<Ptr<GC::GCObject>>(m_value);
}

// 类型获取
Value::Type Value::type() const {
    if (isNil()) return Type::Nil;
    if (isBoolean()) return Type::Boolean;
    if (isNumber()) return Type::Number;
    if (isString()) return Type::String;
    if (isTable()) return Type::Table;
    if (isFunction()) return Type::Function;
    if (isUserData()) return Type::UserData;
    return Type::Nil; // 默认返回nil类型
}

// 比较操作符
bool Value::operator==(const Value& other) const {
    // 不同类型不相等，除了数字可以与字符串比较
    if (type() != other.type()) {
        if (isNumber() && other.isString()) {
            // 尝试将字符串转换为数字
            try {
                f64 otherNum = std::stod(other.asString());
                return asNumber() == otherNum;
            } catch (...) {
                return false;
            }
        } else if (isString() && other.isNumber()) {
            // 尝试将字符串转换为数字
            try {
                f64 thisNum = std::stod(asString());
                return thisNum == other.asNumber();
            } catch (...) {
                return false;
            }
        }
        return false;
    }
    
    // 相同类型，比较值
    switch (type()) {
        case Type::Nil:
            return true; // 所有nil都相等
        case Type::Boolean:
            return asBoolean() == other.asBoolean();
        case Type::Number:
            return asNumber() == other.asNumber();
        case Type::String:
            return asString() == other.asString();
        case Type::Table:
        case Type::Function:
        case Type::UserData:
        case Type::Thread:
            // 引用比较
            return asGCObject() == other.asGCObject();
        default:
            return false;
    }
}

// 字符串表示
Str Value::toString() const {
    std::ostringstream oss;
    
    switch (type()) {
        case Type::Nil:
            oss << "nil";
            break;
            
        case Type::Boolean:
            oss << (asBoolean() ? "true" : "false");
            break;
            
        case Type::Number: {
            f64 num = asNumber();
            if (std::floor(num) == num && std::abs(num) < 1e10) {
                // 整数表示
                oss << static_cast<i64>(num);
            } else {
                // 浮点数表示
                oss << num;
            }
            break;
        }
            
        case Type::String:
            oss << "\"" << asString() << "\"";
            break;
            
        case Type::Table:
            oss << "table: " << asTable().get();
            break;
            
        case Type::Function:
            oss << "function: " << asFunction().get();
            break;
            
        case Type::UserData:
            oss << "userdata: " << asUserData().get();
            break;
            
        default:
            oss << "unknown";
            break;
    }
    
    return oss.str();
}

// 垃圾回收标记
void Value::mark() {
    if (isGCObject()) {
        auto obj = asGCObject();
        if (obj) {
            obj->mark();
        }
    }
}

// 哈希函数，用于HashMap
size_t Value::hash() const {
    switch (type()) {
        case Type::Nil:
            return 0;
            
        case Type::Boolean:
            return std::hash<bool>{}(asBoolean());
            
        case Type::Number:
            return std::hash<f64>{}(asNumber());
            
        case Type::String:
            return std::hash<Str>{}(asString());
            
        case Type::Table:
        case Type::Function:
        case Type::UserData:
        case Type::Thread:
            // 使用指针作为哈希值
            return std::hash<void*>{}(asGCObject().get());
            
        default:
            return 0;
    }
}

} // namespace Object
} // namespace Lua
