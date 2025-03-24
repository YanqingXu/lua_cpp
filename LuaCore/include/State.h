#pragma once

#include "Value.h"
#include "Table.h"
#include "GarbageCollector.h"
#include "types.h"
#include <functional>
#include <stdexcept>

namespace LuaCore {

/**
 * @brief Exception thrown on Lua runtime errors
 */
class LuaException : public std::runtime_error {
public:
    explicit LuaException(const Str& message) : std::runtime_error(message) {}
};

/**
 * @brief Manages the execution state for a Lua instance
 * 
 * The State class represents a complete Lua execution environment, including
 * the value stack, global variables, and runtime configuration. It provides
 * the primary interface for executing Lua code and interacting with C++.
 */
class State : public std::enable_shared_from_this<State> {
public:
    // Create a new Lua state
    static std::shared_ptr<State> create();
    
    // Destructor
    ~State();
    
    // Stack operations
    void push(const Value& value);       // Push a value onto the stack
    Value pop();                         // Pop a value from the stack
    Value& top();                        // Get the top value on the stack
    Value& get(int index);               // Get a value at the specified stack index
    void remove(int index);              // Remove a value at the specified stack index
    int getTop() const;                  // Get the current stack size
    void setTop(int index);              // Set the stack size
    
    // Type checking for stack values
    bool isNil(int index) const;
    bool isBoolean(int index) const;
    bool isNumber(int index) const;
    bool isString(int index) const;
    bool isTable(int index) const;
    bool isFunction(int index) const;
    bool isUserData(int index) const;
    
    // Value retrieval from stack
    bool toBoolean(int index) const;
    double toNumber(int index) const;
    Str toString(int index) const;
    Ptr<Table> toTable(int index) const;
    Ptr<Function> toFunction(int index) const;
    Ptr<UserData> toUserData(int index) const;
    
    // Global table access
    Ptr<Table> getGlobals() const { return m_globals; }
    void setGlobal(const Str& name, const Value& value);
    Value getGlobal(const Str& name) const;
    
    // Function calls
    int call(int nargs, int nresults);
    
    // Registry access (for internal use)
    Ptr<Table> getRegistry() const { return m_registry; }
    
    // Memory management and garbage collection
    GarbageCollector& getGC() { return m_gc; }
    void collectGarbage() { m_gc.collectGarbage(); }
    void collectGarbageIncremental() { m_gc.collectGarbageIncremental(); }
    
    // Create GCObject and register with GC
    template<typename T, typename... Args>
    Ptr<T> createGCObject(Args&&... args) {
        auto obj = make_ptr<T>(std::forward<Args>(args)...);
        m_gc.registerObject(obj);
        return obj;
    }
    
private:
    // Private constructor (use create() instead)
    State();
    
    // Initialize the state
    void initialize();
    
    // Check if a stack index is valid
    bool isValidIndex(int index) const;
    
    // Convert relative index to absolute index
    int absoluteIndex(int index) const;
    
    // Stack of values
    Vec<Value> m_stack;
    
    // Global variables table
    Ptr<Table> m_globals;
    
    // Registry table (for internal use)
    Ptr<Table> m_registry;
    
    // Garbage collector
    GarbageCollector m_gc;
};

} // namespace LuaCore
