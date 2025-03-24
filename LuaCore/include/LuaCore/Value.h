#pragma once

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
    explicit Value(double value) : m_value(value) {}
    explicit Value(const std::string& value) : m_value(value) {}
    
    // Type checking methods
    bool isNil() const { return std::holds_alternative<std::nullptr_t>(m_value); }
    bool isBoolean() const { return std::holds_alternative<bool>(m_value); }
    bool isNumber() const { return std::holds_alternative<double>(m_value); }
    bool isString() const { return std::holds_alternative<std::string>(m_value); }
    bool isTable() const;
    bool isFunction() const;
    bool isUserData() const;

    // Type getter
    Type type() const;
    
    // Value getters with type checking
    bool asBoolean() const;
    double asNumber() const;
    std::string asString() const;
    std::shared_ptr<Table> asTable() const;
    std::shared_ptr<Function> asFunction() const;
    std::shared_ptr<UserData> asUserData() const;

    // Comparison operators
    bool operator==(const Value& other) const;
    bool operator!=(const Value& other) const { return !(*this == other); }
    
    // Conversion to string representation (for debugging/printing)
    std::string toString() const;

private:
    // Core value storage using std::variant
    using ValueVariant = std::variant<
        std::nullptr_t,       // Nil
        bool,                 // Boolean
        double,               // Number
        std::string,          // String
        std::shared_ptr<GCObject>  // Table, Function, UserData (GC-managed types)
    >;
    
    ValueVariant m_value;
};

} // namespace LuaCore
