#pragma once

#include "types.hpp"
#include "gc/gc_object.hpp"

#include <memory>
#include <typeindex>
#include <unordered_map>
#include <functional>

namespace Lua {
namespace Object {

// 前向声明
class Table;

/**
 * @brief Represents user-defined data in Lua
 * 
 * The UserData class allows C++ objects to be stored in Lua and provides
 * metatable support for customizing behavior. It provides type safety and
 * lifecycle management for C++ objects accessible from Lua.
 */
class UserData : public GC::GCObject {
public:
    // Create userdata with optional metatable
    explicit UserData(std::shared_ptr<void> data, 
                     std::type_index type,
                     Ptr<Table> metatable = nullptr);
    
    // Get the metatable
    Ptr<Table> metatable() const { return m_metatable; }
    
    // Set the metatable
    void setMetatable(Ptr<Table> metatable) { m_metatable = metatable; }
    
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
    void mark() override {
        if (isMarked()) return;
        GCObject::mark();
        if (m_metatable) m_metatable->mark();
    }
    
private:
    std::shared_ptr<void> m_data;      // Type-erased data pointer
    std::type_index m_typeInfo;        // Type information for runtime checks
    Ptr<Table> m_metatable;    // Optional metatable for this userdata
};

} // namespace Object
} // namespace Lua
