#pragma once

#include "GCObject.h"
#include "Value.h"
#include <vector>
#include <string>
#include <functional>
#include <memory>

namespace LuaCore {

// Forward declarations
class State;

/**
 * @brief Represents a Lua function prototype
 * 
 * The Prototype class stores compiled bytecode and metadata for a Lua function,
 * including constant values, upvalue information, and debug information.
 */
class Prototype {
public:
    Prototype(const std::string& name = "");
    
    // Basic properties
    const std::string& name() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }
    
    // Debug information
    int getLineNumber(int pc) const;
    void setLineNumber(int pc, int line);
    
    // Source code information
    const std::string& getSource() const { return m_source; }
    void setSource(const std::string& source) { m_source = source; }
    
private:
    std::string m_name;                // Function name
    std::string m_source;              // Source code
    std::vector<int> m_lineNumbers;    // Line number information
    int m_firstLine = 0;               // First line in source
    int m_lastLine = 0;                // Last line in source
};

/**
 * @brief Base class for all callable function objects
 * 
 * The Function class is the base for both Lua functions and C++ functions that can
 * be called from Lua code. It provides the common interface for function calls.
 */
class Function : public GCObject {
public:
    virtual ~Function() = default;
    
    // Function call interface (to be implemented by derived classes)
    virtual int call(State* state, int nargs, int nresults) = 0;
    
    // GCObject interface
    Type type() const override { return Type::Closure; }
    
protected:
    Function() = default;
};

/**
 * @brief Represents a Lua closure
 * 
 * A Closure is a function together with its associated upvalues (captured variables).
 * It can be either a Lua function (with bytecode) or a C++ function.
 */
class Closure : public Function {
public:
    // Create a Lua function closure
    explicit Closure(std::shared_ptr<Prototype> proto);
    
    // Create a C++ function closure
    explicit Closure(std::function<int(State*)> func);
    
    // Function call implementation
    int call(State* state, int nargs, int nresults) override;
    
    // GCObject interface
    void mark() override;
    
    // Access to prototype
    std::shared_ptr<Prototype> prototype() const { return m_prototype; }
    
    // Check if this is a C++ function
    bool isCFunction() const { return static_cast<bool>(m_cFunction); }
    
private:
    std::shared_ptr<Prototype> m_prototype;      // Function prototype (for Lua functions)
    std::function<int(State*)> m_cFunction;      // C++ function (for C++ functions)
    std::vector<std::shared_ptr<Value>> m_upvalues; // Upvalues (captured variables)
};

} // namespace LuaCore
