#pragma once

#include "GCObject.h"
#include "Value.h"
#include "types.h"
#include <functional>

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
    Prototype(const Str& name = "");
    
    // Basic properties
    const Str& name() const { return m_name; }
    void setName(const Str& name) { m_name = name; }
    
    // Debug information
    i32 getLineNumber(i32 pc) const;
    void setLineNumber(i32 pc, i32 line);
    
    // Source code information
    const Str& getSource() const { return m_source; }
    void setSource(const Str& source) { m_source = source; }
    
private:
    Str m_name;                // Function name
    Str m_source;              // Source code
    Vec<i32> m_lineNumbers;    // Line number information
    i32 m_firstLine = 0;               // First line in source
    i32 m_lastLine = 0;                // Last line in source
};

/**
 * @brief Base class for all callable function objects
 * 
 * The Function class is the base for all function objects that can
 * be called from Lua code. It provides the common interface for function calls.
 */
class Function : public GCObject {
public:
    virtual ~Function() = default;
    
    // Function call interface (to be implemented by derived classes)
    virtual i32 call(State* state, i32 nargs, i32 nresults) = 0;
    
    // GCObject interface
    Type type() const override { return Type::Closure; }
    void mark() override { GCObject::mark(); }
    
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
    explicit Closure(Ptr<Prototype> proto);
    
    // Create a C++ function closure
    explicit Closure(std::function<i32(State*)> func);
    
    // Function call implementation
    i32 call(State* state, i32 nargs, i32 nresults) override;
    
    // GCObject interface
    void mark() override;
    
    // Access to prototype
    Ptr<Prototype> prototype() const { return m_prototype; }
    
    // Check if this is a C++ function
    bool isCFunction() const { return static_cast<bool>(m_cFunction); }
    
private:
    Ptr<Prototype> m_prototype;      // Function prototype (for Lua functions)
    std::function<i32(State*)> m_cFunction;      // C++ function (for C++ functions)
    Vec<Ptr<Value>> m_upvalues; // Upvalues (captured variables)
};

} // namespace LuaCore
