#pragma once

#include "GCObject.h"
#include "types.h"
#include <string>

namespace LuaCore {

/**
 * @brief Represents an immutable string object in Lua
 * 
 * The String class is a garbage-collectible object that represents a Lua string.
 * It provides efficient storage and string operations while participating in the
 * garbage collection system.
 */
class String : public GCObject {
public:
    // Create a string from a C++ string
    explicit String(const Str& value) : m_value(value) {}
    
    // Create a string from a C string
    explicit String(const char* value) : m_value(value) {}
    
    // Get the string value
    const Str& value() const { return m_value; }
    
    // Get the length of the string
    size_t length() const { return m_value.length(); }
    
    // Comparison operators
    bool operator==(const String& other) const { return m_value == other.m_value; }
    bool operator!=(const String& other) const { return m_value != other.m_value; }
    
    // String concatenation
    Ptr<String> concat(const Ptr<String>& other) const {
        return makePtr<String>(m_value + other->m_value);
    }
    
    // GCObject interface implementation
    Type type() const override { return Type::String; }
    void mark() override { GCObject::mark(); }
    
private:
    Str m_value;  // The actual string content
};

} // namespace LuaCore
