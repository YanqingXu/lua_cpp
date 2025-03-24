#pragma once

#include "types.h"
#include <string>
#include <variant>
#include <memory>
#include <functional>
#include <unordered_map>

namespace LuaCore {

// Forward declarations
class Table;
class Function;
class Closure;
class UserData;
class GCObject;

/**
 * @brief Represents all possible Lua value types
 * 
 * The Value class uses std::variant to represent all possible Lua types in a type-safe
 * manner, while allowing efficient storage and easy type checking.
 */
class Value {
public:
    // Lua value type enumeration
    enum class Type {
        Nil,
        Boolean,
        Number,
        String,
        Table,
        Function,
        UserData
    };

    // Default constructor creates a nil value
    Value() : m_value(nullptr) {}

    // Constructors for different types
    explicit Value(std::nullptr_t) : m_value(nullptr) {}
    explicit Value(bool value) : m_value(value) {}
    explicit Value(f64 value) : m_value(value) {}
    explicit Value(const Str& value) : m_value(value) {}
    
    // Type checking methods
    bool isNil() const { return std::holds_alternative<std::nullptr_t>(m_value); }
    bool isBoolean() const { return std::holds_alternative<bool>(m_value); }
    bool isNumber() const { return std::holds_alternative<f64>(m_value); }
    bool isString() const { return std::holds_alternative<Str>(m_value); }
    bool isTable() const;
    bool isFunction() const;
    bool isUserData() const;

    // Type getter
    Type type() const;
    
    // Value getters with type checking
    bool asBoolean() const;
    f64 asNumber() const;
    Str asString() const;
    Ptr<Table> asTable() const;
    Ptr<Function> asFunction() const;
    Ptr<UserData> asUserData() const;

    // Comparison operators
    bool operator==(const Value& other) const;
    bool operator!=(const Value& other) const { return !(*this == other); }
    
    // Conversion to string representation (for debugging/printing)
    Str toString() const;

private:
    // Core value storage using std::variant
    using ValueVariant = std::variant<
        std::nullptr_t,       // Nil
        bool,               // Boolean
        f64,                  // Number
        Str,                  // String
        Ptr<GCObject>         // Table, Function, UserData (GC-managed types)
    >;
    
    ValueVariant m_value;
};

} // namespace LuaCore
