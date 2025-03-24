#include "Function.h"
#include "State.h"

namespace LuaCore {

// Prototype implementation
Prototype::Prototype(const Str& name) : m_name(name) {
}

i32 Prototype::getLineNumber(i32 pc) const {
    if (pc < 0 || pc >= static_cast<i32>(m_lineNumbers.size())) {
        return -1;
    }
    return m_lineNumbers[pc];
}

void Prototype::setLineNumber(i32 pc, i32 line) {
    if (pc >= static_cast<i32>(m_lineNumbers.size())) {
        m_lineNumbers.resize(pc + 1, 0);
    }
    m_lineNumbers[pc] = line;
    
    // Update line range
    if (line > 0) {
        if (m_firstLine == 0 || line < m_firstLine) {
            m_firstLine = line;
        }
        if (line > m_lastLine) {
            m_lastLine = line;
        }
    }
}

// Closure implementation
Closure::Closure(Ptr<Prototype> proto) 
    : m_prototype(proto), m_cFunction(nullptr) {
}

Closure::Closure(std::function<i32(State*)> func)
    : m_prototype(nullptr), m_cFunction(func) {
}

i32 Closure::call(State* state, i32 nargs, i32 nresults) {
    if (isCFunction()) {
        // Call C++ function directly
        return m_cFunction(state);
    }
    else {
        // Lua function - this will be implemented by the VM later
        // For now, just return an error
        throw std::runtime_error("Lua function execution not yet implemented");
    }
}

void Closure::mark() {
    // Skip if already marked
    if (isMarked()) {
        return;
    }
    
    // Mark this object
    GCObject::mark();
    
    // Mark upvalues
    for (auto& upvalue : m_upvalues) {
        if (upvalue) {
            // Check if upvalue contains a GC object
            if (upvalue->isTable()) {
                upvalue->asTable()->mark();
            }
            else if (upvalue->isFunction()) {
                upvalue->asFunction()->mark();
            }
            else if (upvalue->isUserData()) {
                upvalue->asUserData()->mark();
            }
        }
    }
}

} // namespace LuaCore
