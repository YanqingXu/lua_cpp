#pragma once

#include "GCObject.h"
#include "Value.h"
#include <unordered_map>
#include <vector>
#include <memory>

namespace LuaCore {

/**
 * @brief Implements the Lua table data structure
 * 
 * The Table class represents Lua tables, which are the primary data structure in Lua.
 * It supports both array and hash parts for efficient storage of sequential and
 * associative data. Tables are garbage-collectible objects.
 */
class Table : public GCObject {
public:
    // Create an empty table
    Table();
    
    // Get value by key
    Value get(const Value& key) const;
    
    // Set value for a key
    void set(const Value& key, const Value& value);
    
    // Get the length of the array part
    std::size_t length() const;
    
    // Check if a key exists in the table
    bool contains(const Value& key) const;
    
    // Get raw array access for optimization
    const std::vector<Value>& getArrayPart() const { return m_array; }
    
    // GCObject interface implementation
    Type type() const override { return Type::Table; }
    void mark() override;
    
    // Iterator support
    using Iterator = std::unordered_map<Value, Value>::const_iterator;
    Iterator begin() const { return m_hash.begin(); }
    Iterator end() const { return m_hash.end(); }
    
private:
    // Array part for sequential indices (1-based in Lua, 0-based in implementation)
    std::vector<Value> m_array;
    
    // Hash part for non-sequential keys
    std::unordered_map<Value, Value> m_hash;
    
    // Utility to determine if a key should be in the array part
    bool isArrayIndex(const Value& key) const;
    
    // Resize array part if needed
    void resizeArrayIfNeeded();
};

} // namespace LuaCore
