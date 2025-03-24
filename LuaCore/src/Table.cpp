#include "LuaCore/Table.h"
#include <algorithm>
#include <cmath>

namespace LuaCore {

Table::Table() : GCObject() {
    // Initialize with a reasonable size for the array part
    m_array.reserve(8);
}

Value Table::get(const Value& key) const {
    // Check if key is an integer index for array part
    if (isArrayIndex(key)) {
        try {
            double numKey = key.asNumber();
            size_t index = static_cast<size_t>(numKey) - 1; // Convert from 1-based to 0-based
            
            if (index < m_array.size()) {
                return m_array[index];
            }
        }
        catch (const std::exception&) {
            // Not a valid number, fall through to hash lookup
        }
    }
    
    // Look up in the hash part
    auto it = m_hash.find(key);
    if (it != m_hash.end()) {
        return it->second;
    }
    
    // Key not found
    return Value(); // Return nil
}

void Table::set(const Value& key, const Value& value) {
    // Check if we're setting nil, which means removal
    bool isNilValue = value.isNil();
    
    // Check if key is a suitable index for array part
    if (isArrayIndex(key)) {
        try {
            double numKey = key.asNumber();
            // Check if it's an integer
            if (std::floor(numKey) == numKey && numKey > 0) {
                size_t index = static_cast<size_t>(numKey) - 1; // Convert from 1-based to 0-based
                
                // Handle array part
                if (index < m_array.size()) {
                    if (isNilValue) {
                        // Setting to nil - only remove if it's at the end of the array
                        if (index == m_array.size() - 1) {
                            // Remove from the end
                            m_array.pop_back();
                            // Also remove any preceding nils
                            while (!m_array.empty() && m_array.back().isNil()) {
                                m_array.pop_back();
                            }
                        }
                        else {
                            // Internal nil - just set to nil
                            m_array[index] = Value();
                        }
                    }
                    else {
                        // Normal value assignment
                        m_array[index] = value;
                    }
                    return;
                }
                else if (!isNilValue) {
                    // Extending array part - only if the index is not too far
                    if (index < m_array.size() + 16) {
                        // Resize the array to accommodate the new index
                        m_array.resize(index + 1);
                        m_array[index] = value;
                        return;
                    }
                    // Otherwise, fall through to hash table
                }
                else {
                    // Setting a nil value beyond current array bounds
                    // No need to do anything as the default is nil
                    return;
                }
            }
        }
        catch (const std::exception&) {
            // Not a valid number, fall through to hash handling
        }
    }
    
    // Handle hash part
    if (isNilValue) {
        // Remove the key if the value is nil
        m_hash.erase(key);
    }
    else {
        // Set or update the value in the hash table
        m_hash[key] = value;
    }
}

std::size_t Table::length() const {
    // Find the boundary between non-nil and nil values in the array part
    // This implements the Lua length operator behavior
    
    // First check the array part
    size_t len = m_array.size();
    while (len > 0 && m_array[len - 1].isNil()) {
        len--;
    }
    
    // Check if any hash entries extend the sequence
    bool foundGap = false;
    size_t i = len + 1;
    while (!foundGap) {
        Value key(static_cast<double>(i));
        auto it = m_hash.find(key);
        
        if (it == m_hash.end() || it->second.isNil()) {
            foundGap = true;
        }
        else {
            len = i;
            i++;
        }
    }
    
    return len;
}

bool Table::contains(const Value& key) const {
    // Check if the key exists in either the array or hash part
    
    if (isArrayIndex(key)) {
        try {
            double numKey = key.asNumber();
            size_t index = static_cast<size_t>(numKey) - 1; // Convert from 1-based to 0-based
            
            // Check array part
            if (index < m_array.size() && !m_array[index].isNil()) {
                return true;
            }
        }
        catch (const std::exception&) {
            // Not a valid number, fall through to hash check
        }
    }
    
    // Check hash part
    return m_hash.find(key) != m_hash.end();
}

void Table::mark() {
    // Skip if already marked
    if (isMarked()) {
        return;
    }
    
    // Mark this object
    GCObject::mark();
    
    // Mark all values in the array part
    for (auto& value : m_array) {
        // Mark tables, functions, and userdata
        if (value.isTable()) {
            value.asTable()->mark();
        }
        else if (value.isFunction()) {
            value.asFunction()->mark();
        }
        else if (value.isUserData()) {
            value.asUserData()->mark();
        }
    }
    
    // Mark all keys and values in the hash part
    for (auto& [key, value] : m_hash) {
        // Mark keys that are GCObjects
        if (key.isTable()) {
            key.asTable()->mark();
        }
        else if (key.isFunction()) {
            key.asFunction()->mark();
        }
        else if (key.isUserData()) {
            key.asUserData()->mark();
        }
        
        // Mark values that are GCObjects
        if (value.isTable()) {
            value.asTable()->mark();
        }
        else if (value.isFunction()) {
            value.asFunction()->mark();
        }
        else if (value.isUserData()) {
            value.asUserData()->mark();
        }
    }
}

bool Table::isArrayIndex(const Value& key) const {
    // Check if key is a number that could be an array index
    if (!key.isNumber()) {
        return false;
    }
    
    double num = key.asNumber();
    return num > 0 && std::floor(num) == num && num <= 1e9; // Reasonable upper bound
}

void Table::resizeArrayIfNeeded() {
    // Check if we should move values from the hash part to the array part
    size_t arraySize = m_array.size();
    size_t newSize = arraySize;
    
    // Check up to 16 slots beyond current array size
    for (size_t i = arraySize + 1; i <= arraySize + 16; i++) {
        Value key(static_cast<double>(i));
        auto it = m_hash.find(key);
        
        if (it != m_hash.end()) {
            newSize = i; // Extend to include this index
        }
    }
    
    if (newSize > arraySize) {
        // Resize array and move values from hash
        size_t oldSize = m_array.size();
        m_array.resize(newSize);
        
        // Fill in values
        for (size_t i = oldSize + 1; i <= newSize; i++) {
            Value key(static_cast<double>(i));
            auto it = m_hash.find(key);
            
            if (it != m_hash.end()) {
                m_array[i - 1] = it->second;
                m_hash.erase(it);
            }
        }
    }
}

} // namespace LuaCore
