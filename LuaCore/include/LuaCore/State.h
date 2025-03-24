#pragma once

#include "Value.h"
#include "Table.h"
#include <vector>
#include <memory>
#include <functional>
#include <stdexcept>

namespace LuaCore {

/**
 * @brief Exception thrown on Lua runtime errors
 */
class LuaException : public std::runtime_error {
public:
    explicit LuaException(const std::string& message) : std::runtime_error(message) {}
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
    std::string toString(int index) const;
    std::shared_ptr<Table> toTable(int index) const;
    std::shared_ptr<Function> toFunction(int index) const;
    std::shared_ptr<UserData> toUserData(int index) const;
    
    // Global table access
    std::shared_ptr<Table> getGlobals() const { return m_globals; }
    void setGlobal(const std::string& name, const Value& value);
    Value getGlobal(const std::string& name) const;
    
    // Function calls
    int call(int nargs, int nresults);
    
    // Registry access (for internal use)
    std::shared_ptr<Table> getRegistry() const { return m_registry; }
    
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
    std::vector<Value> m_stack;
    
    // Global environment table
    std::shared_ptr<Table> m_globals;
    
    // Registry table (for internal use)
    std::shared_ptr<Table> m_registry;
};

} // namespace LuaCore
