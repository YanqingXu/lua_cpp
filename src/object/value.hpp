#pragma once

#include "types.hpp"
#include "gc/gc_object.hpp"
#include "object/object_types.hpp"
#include "string.hpp"
#include "table.hpp"
#include "function.hpp"
#include "userdata.hpp"
#include "thread.hpp"

#include <variant>

namespace Lua {

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
    explicit Value(Ptr<Thread> value);

    // Static factory methods for creating values
    static Value nil();
    static Value boolean(bool value);
    static Value number(f64 value);
    static Value string(const Str& value);
    static Value string(Ptr<String> value);
    static Value table(Ptr<Table> value);
    static Value function(Ptr<Function> value);
    static Value userdata(Ptr<UserData> value);
    static Value thread(Ptr<Thread> value);

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
    bool isThread() const;
    bool isGCObject() const;
    
    // Type conversion methods
    bool asBoolean() const;
    f64 asNumber() const;
    Str asString() const;
    Ptr<String> asStringObject() const;
    Ptr<Table> asTable() const;
    Ptr<Function> asFunction() const;
    Ptr<UserData> asUserData() const;
    Ptr<Thread> asThread() const;
    Ptr<GCObject> asGCObject() const;
    
    // Comparison operators
    bool operator==(const Value& other) const;
    bool operator!=(const Value& other) const { return !(*this == other); }
    
    // String representation
    Str toString() const;
    
    // Marking for garbage collection
    void mark(GarbageCollector* gc);
    
    // For use as a key in hash tables
    size_t hash() const;
    
private:
    // The actual value storage using variant
    using ValueVariant = ::std::variant<
        bool,                        // boolean
        i64,                         // integer
        f64,                         // float
        Ptr<String>,                 // string
        Ptr<Table>,                  // table
        Ptr<Function>,               // function
        Ptr<UserData>,               // userdata
        Ptr<Thread>                  // thread
    >;
    
    ValueVariant m_value;
};

// Hash function for Value (for use in unordered_map)
struct ValueHash {
    size_t operator()(const Value& v) const {
        return v.hash();
    }
};
} // namespace Lua
