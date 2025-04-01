#pragma once

#include "types.hpp"
#include "gc/gc_object.hpp"

#include <variant>

namespace Lua {
namespace Object {

// Forward declarations
class Table;
class Function;
class Closure;
class UserData;
class String;

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
        UserData,
        Thread
    };

    // Default constructor creates a nil value
    Value() : m_value() {}

    // Constructors for different types
    explicit Value(bool value) : m_value(value) {}
    explicit Value(f64 value) : m_value(value) {}
    explicit Value(const Str& value);
    explicit Value(Ptr<String> value);
    explicit Value(Ptr<Table> value);
    explicit Value(Ptr<Function> value);
    explicit Value(Ptr<UserData> value);

    // Static factory methods for creating values
    static Value nil();
    static Value boolean(bool value);
    static Value number(f64 value);
    static Value string(const Str& value);
    static Value string(Ptr<String> value);
    static Value table(Ptr<Table> value);
    static Value function(Ptr<Function> value);
    static Value userdata(Ptr<UserData> value);

    // Type getter
    Type type() const;
    
    // Type checking methods
    bool isNil() const;
    bool isBoolean() const;
    bool isNumber() const;
    bool isString() const;
    bool isTable() const;
    bool isFunction() const;
    bool isUserData() const;
    bool isThread() const { return false; } // 未实现线程
    bool isGCObject() const;
    
    // Type conversion methods
    bool asBoolean() const;
    f64 asNumber() const;
    Str asString() const;
    Ptr<String> asStringObject() const;
    Ptr<Table> asTable() const;
    Ptr<Function> asFunction() const;
    Ptr<UserData> asUserData() const;
    Ptr<GC::GCObject> asGCObject() const;
    
    // Comparison operators
    bool operator==(const Value& other) const;
    bool operator!=(const Value& other) const { return !(*this == other); }
    
    // String representation
    Str toString() const;
    
    // Marking for garbage collection
    void mark();
    
    // For use as a key in hash tables
    size_t hash() const;
    
private:
    // The actual value storage using variant
    using ValueVariant = ::std::variant<
        bool,                // Boolean
        f64,                 // Number
		Str,                 // String
        Ptr<GC::GCObject>    // GC Objects (String, Table, Function, UserData)
    >;
    
    ValueVariant m_value;
};

// Hash function for Value (for use in unordered_map)
struct ValueHash {
    size_t operator()(const Value& v) const {
        return v.hash();
    }
};

} // namespace Object
} // namespace Lua
