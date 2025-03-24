#pragma once

#include "GCObject.h"
#include "Table.h"
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <functional>

namespace LuaCore {

/**
 * @brief Represents user-defined data in Lua
 * 
 * The UserData class allows C++ objects to be stored in Lua and provides
 * metatable support for customizing behavior. It provides type safety and
 * lifecycle management for C++ objects accessible from Lua.
 */
class UserData : public GCObject {
public:
    // Create userdata with optional metatable
    explicit UserData(std::shared_ptr<void> data, 
                     std::type_index type,
                     std::shared_ptr<Table> metatable = nullptr);
    
    // Get the metatable
    std::shared_ptr<Table> metatable() const { return m_metatable; }
    
    // Set the metatable
    void setMetatable(std::shared_ptr<Table> metatable) { m_metatable = metatable; }
    
    // Get raw data pointer (type-erased)
    void* rawData() const { return m_data.get(); }
    
    // Get data as specific type (with type checking)
    template<typename T>
    T* as() const {
        if (std::type_index(typeid(T)) != m_typeInfo) {
            throw std::runtime_error("UserData type mismatch");
        }
        return static_cast<T*>(m_data.get());
    }
    
    // Check if userdata is of a specific type
    template<typename T>
    bool is() const {
        return std::type_index(typeid(T)) == m_typeInfo;
    }
    
    // GCObject interface
    Type type() const override { return Type::UserData; }
    void mark() override;
    
private:
    std::shared_ptr<void> m_data;          // Type-erased pointer to the actual data
    std::type_index m_typeInfo;            // Type information for runtime type checking
    std::shared_ptr<Table> m_metatable;    // Optional metatable
};

} // namespace LuaCore
