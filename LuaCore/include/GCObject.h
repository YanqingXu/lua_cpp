#pragma once

#include <memory>

namespace LuaCore {

/**
 * @brief Base class for all garbage-collectible objects in Lua
 * 
 * GCObject serves as the base class for all Lua objects that are managed by
 * the garbage collector, including Tables, Functions, and UserData.
 * It provides common functionality for garbage collection marking and
 * lifecycle management.
 */
class GCObject : public std::enable_shared_from_this<GCObject> {
public:
    // Virtual destructor to ensure proper cleanup in derived classes
    virtual ~GCObject() = default;
    
    // Garbage collection support
    virtual void mark() { m_marked = true; }
    bool isMarked() const { return m_marked; }
    void unmark() { m_marked = false; }
    
    // Object type enumeration
    enum class Type {
        Table,
        Closure,
        UserData
    };
    
    // Get the type of this GCObject (to be implemented by derived classes)
    virtual Type type() const = 0;
    
protected:
    // Protected constructor to prevent direct instantiation of GCObject
    GCObject() : m_marked(false) {}
    
private:
    bool m_marked; // Mark flag for garbage collection
};

} // namespace LuaCore
