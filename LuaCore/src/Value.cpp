#include "LuaCore/Value.h"
#include "LuaCore/Table.h"
#include "LuaCore/Function.h"
#include "LuaCore/UserData.h"
#include <sstream>
#include <iomanip>

namespace LuaCore {

// Type getter implementation
Value::Type Value::type() const {
    if (isNil()) return Type::Nil;
    if (isBoolean()) return Type::Boolean;
    if (isNumber()) return Type::Number;
    if (isString()) return Type::String;
    if (isTable()) return Type::Table;
    if (isFunction()) return Type::Function;
    return Type::UserData; // Must be UserData if none of the above
}

// Advanced type checking for GCObject types
bool Value::isTable() const {
    if (!std::holds_alternative<std::shared_ptr<GCObject>>(m_value)) {
        return false;
    }
    
    auto obj = std::get<std::shared_ptr<GCObject>>(m_value);
    return obj && obj->type() == GCObject::Type::Table;
}

bool Value::isFunction() const {
    if (!std::holds_alternative<std::shared_ptr<GCObject>>(m_value)) {
        return false;
    }
    
    auto obj = std::get<std::shared_ptr<GCObject>>(m_value);
    return obj && obj->type() == GCObject::Type::Closure;
}

bool Value::isUserData() const {
    if (!std::holds_alternative<std::shared_ptr<GCObject>>(m_value)) {
        return false;
    }
    
    auto obj = std::get<std::shared_ptr<GCObject>>(m_value);
    return obj && obj->type() == GCObject::Type::UserData;
}

// Value getters with type checking
bool Value::asBoolean() const {
    if (!isBoolean()) {
        throw std::runtime_error("Value is not a boolean");
    }
    return std::get<bool>(m_value);
}

double Value::asNumber() const {
    if (!isNumber()) {
        throw std::runtime_error("Value is not a number");
    }
    return std::get<double>(m_value);
}

std::string Value::asString() const {
    if (!isString()) {
        throw std::runtime_error("Value is not a string");
    }
    return std::get<std::string>(m_value);
}

std::shared_ptr<Table> Value::asTable() const {
    if (!isTable()) {
        throw std::runtime_error("Value is not a table");
    }
    return std::static_pointer_cast<Table>(
        std::get<std::shared_ptr<GCObject>>(m_value)
    );
}

std::shared_ptr<Function> Value::asFunction() const {
    if (!isFunction()) {
        throw std::runtime_error("Value is not a function");
    }
    return std::static_pointer_cast<Function>(
        std::get<std::shared_ptr<GCObject>>(m_value)
    );
}

std::shared_ptr<UserData> Value::asUserData() const {
    if (!isUserData()) {
        throw std::runtime_error("Value is not a userdata");
    }
    return std::static_pointer_cast<UserData>(
        std::get<std::shared_ptr<GCObject>>(m_value)
    );
}

// Comparison operators
bool Value::operator==(const Value& other) const {
    // Different types are never equal (except for special cases)
    if (type() != other.type()) {
        return false;
    }
    
    // Compare based on type
    switch (type()) {
        case Type::Nil:
            return true; // All nil values are equal
        case Type::Boolean:
            return asBoolean() == other.asBoolean();
        case Type::Number:
            return asNumber() == other.asNumber();
        case Type::String:
            return asString() == other.asString();
        case Type::Table:
        case Type::Function:
        case Type::UserData:
            // For reference types, compare by identity (pointer equality)
            if (std::holds_alternative<std::shared_ptr<GCObject>>(m_value) &&
                std::holds_alternative<std::shared_ptr<GCObject>>(other.m_value)) {
                return std::get<std::shared_ptr<GCObject>>(m_value) ==
                       std::get<std::shared_ptr<GCObject>>(other.m_value);
            }
            return false;
        default:
            return false;
    }
}

// Conversion to string representation (for debugging/printing)
std::string Value::toString() const {
    std::ostringstream oss;
    
    switch (type()) {
        case Type::Nil:
            oss << "nil";
            break;
        case Type::Boolean:
            oss << (asBoolean() ? "true" : "false");
            break;
        case Type::Number:
            oss << std::fixed << std::setprecision(14) << asNumber();
            // Remove trailing zeros and decimal point if it's an integer
            std::string str = oss.str();
            if (str.find('.') != std::string::npos) {
                str.erase(str.find_last_not_of('0') + 1, std::string::npos);
                if (str.back() == '.') {
                    str.pop_back();
                }
            }
            return str;
        case Type::String:
            oss << "\"" << asString() << "\"";
            break;
        case Type::Table:
            oss << "table: " << std::hex << asTable().get();
            break;
        case Type::Function:
            oss << "function: " << std::hex << asFunction().get();
            break;
        case Type::UserData:
            oss << "userdata: " << std::hex << asUserData().get();
            break;
    }
    
    return oss.str();
}

} // namespace LuaCore
