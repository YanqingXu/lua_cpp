#include "Value.h"
#include "Table.h"
#include "Function.h"
#include "UserData.h"
#include "types.h"
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
    if (!std::holds_alternative<Ptr<GCObject>>(m_value)) {
        return false;
    }
    
    auto obj = std::get<Ptr<GCObject>>(m_value);
    return obj && obj->type() == GCObject::Type::Table;
}

bool Value::isFunction() const {
    if (!std::holds_alternative<Ptr<GCObject>>(m_value)) {
        return false;
    }
    
    auto obj = std::get<Ptr<GCObject>>(m_value);
    return obj && obj->type() == GCObject::Type::Closure;
}

bool Value::isUserData() const {
    if (!std::holds_alternative<Ptr<GCObject>>(m_value)) {
        return false;
    }
    
    auto obj = std::get<Ptr<GCObject>>(m_value);
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

Str Value::asString() const {
    if (!isString()) {
        throw std::runtime_error("Value is not a string");
    }
    return std::get<Str>(m_value);
}

Ptr<Table> Value::asTable() const {
    if (!isTable()) {
        throw std::runtime_error("Value is not a table");
    }
    return std::static_pointer_cast<Table>(
        std::get<Ptr<GCObject>>(m_value)
    );
}

Ptr<Function> Value::asFunction() const {
    if (!isFunction()) {
        throw std::runtime_error("Value is not a function");
    }
    return std::static_pointer_cast<Function>(
        std::get<Ptr<GCObject>>(m_value)
    );
}

Ptr<UserData> Value::asUserData() const {
    if (!isUserData()) {
        throw std::runtime_error("Value is not a userdata");
    }
    return std::static_pointer_cast<UserData>(
        std::get<Ptr<GCObject>>(m_value)
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
            if (std::holds_alternative<Ptr<GCObject>>(m_value) &&
                std::holds_alternative<Ptr<GCObject>>(other.m_value)) {
                return std::get<Ptr<GCObject>>(m_value) ==
                       std::get<Ptr<GCObject>>(other.m_value);
            }
            return false;
        default:
            return false;
    }
}

// Conversion to string representation (for debugging/printing)
Str Value::toString() const {
    std::ostringstream oss;
	Str str;

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
            str = oss.str();
            if (str.find('.') != Str::npos) {
                str.erase(str.find_last_not_of('0') + 1, Str::npos);
                if (str.back() == '.') {
                    str.pop_back();
                }
            }
            break;
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

	if (str.empty()) {
		str = oss.str();
	}

    return str;
}

} // namespace LuaCore
