#pragma once

#include "types.hpp"
#include "object/value.hpp"
#include "object/table.hpp"
#include "gc/garbage_collector.hpp"

#include <functional>
#include <stdexcept>
#include <memory>
#include <vector>

namespace Lua {
namespace VM {

/**
 * @brief Exception thrown on Lua runtime errors
 */
class LuaException : public std::runtime_error {
public:
    explicit LuaException(const Str& message) : std::runtime_error(message) {}
};

// 前向声明
class VM;

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
    
    // Open standard libraries
    void openLibs();
    
    // Execute Lua code
    i32 doString(const Str& code);
    i32 doFile(const Str& filename);
    
    // Stack operations
    void push(const Object::Value& value);        // Push a value onto the stack
    void pushNil();                               // Push nil onto the stack
    void pushBoolean(bool b);                     // Push boolean onto the stack
    void pushNumber(double n);                    // Push number onto the stack
    void pushString(const Str& s);                // Push string onto the stack
    void pushTable(Ptr<Object::Table> table);     // Push table onto the stack
    void pushFunction(Ptr<Object::Function> function); // Push function onto the stack
    
    Object::Value pop();                          // Pop a value from the stack
    void pop(i32 n);                              // Pop n values from the stack
    Object::Value peek(i32 index) const;          // Get a value at the specified stack index
    i32 getTop() const;                           // Get the current stack size
    void setTop(i32 index);                       // Set the stack size
    bool checkStack(i32 n);                       // Ensure stack has space for n more elements
    i32 absIndex(i32 index) const;                // Convert a relative index to absolute
    
    // Table operations
    void createTable(i32 narray = 0, i32 nrec = 0); // Create a new table and push it
    void getTable(i32 index);                     // table[key] where key is at top of stack
    void setTable(i32 index);                     // table[key] = value where key and value are at top
    void getField(i32 index, const Str& k);       // table[k]
    void setField(i32 index, const Str& k);       // table[k] = v where v is at top
    void rawGetI(i32 index, i32 i);               // table[i] without metamethods
    void rawSetI(i32 index, i32 i);               // table[i] = v without metamethods
    
    // Global variables
    void getGlobal(const Str& name);              // Push _G[name]
    void setGlobal(const Str& name);              // _G[name] = v where v is at top
    
    // Type checking for stack values
    bool isNil(i32 index) const;
    bool isBoolean(i32 index) const;
    bool isNumber(i32 index) const;
    bool isString(i32 index) const;
    bool isTable(i32 index) const;
    bool isFunction(i32 index) const;
    bool isUserData(i32 index) const;
    bool isThread(i32 index) const;
    
    // Stack value conversion
    bool toBoolean(i32 index) const;
    double toNumber(i32 index) const;
    Str toString(i32 index) const;
    Ptr<Object::Table> toTable(i32 index) const;
    Ptr<Object::Function> toFunction(i32 index) const;
    Ptr<Object::UserData> toUserData(i32 index) const;
    
    // C++ function management
    using CFunction = std::function<int(State*)>;
    void registerFunction(const Str& name, CFunction func);
    i32 call(i32 nargs, i32 nresults);
    
    // Get the garbage collector
    GC::GarbageCollector& gc() { return *m_gc; }
    
    // Error handling
    void error(const Str& message);
    
    // Registry (private storage for C++ code)
    Ptr<Object::Table> getRegistry() const { return m_registry; }
    
private:
    // Private constructor (use create() instead)
    State();
    
    // Initialize the state
    void initialize();
    
    // Stack implementation
    std::vector<Object::Value> m_stack;
    i32 m_stackTop;
    
    // Global environment and registry
    Ptr<Object::Table> m_globals;
    Ptr<Object::Table> m_registry;
    
    // Garbage collector
    std::unique_ptr<GC::GarbageCollector> m_gc;
    
    // Virtual machine
    std::shared_ptr<VM> m_vm;
    
    // Call stack tracking
    i32 m_callDepth;
    
    // Friends
    friend class VM;
    friend class GC::GarbageCollector;
};

} // namespace VM
} // namespace Lua
