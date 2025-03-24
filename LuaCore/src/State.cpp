#include "LuaCore/State.h"
#include "LuaCore/Function.h"
#include "LuaCore/UserData.h"
#include <algorithm>

namespace LuaCore {

// Static method to create a new state
std::shared_ptr<State> State::create() {
    auto state = std::shared_ptr<State>(new State());
    state->initialize();
    return state;
}

// Constructor
State::State() {
    // Reserve space for the stack to avoid frequent reallocations
    m_stack.reserve(16);
}

// Destructor
State::~State() {
    // Clear the stack
    m_stack.clear();
    
    // Tables will be cleaned up by shared_ptr
}

// Initialize the state
void State::initialize() {
    // Create global and registry tables
    m_globals = std::make_shared<Table>();
    m_registry = std::make_shared<Table>();
}

// Stack operations
void State::push(const Value& value) {
    m_stack.push_back(value);
}

Value State::pop() {
    if (m_stack.empty()) {
        throw LuaException("stack underflow");
    }
    
    Value value = m_stack.back();
    m_stack.pop_back();
    return value;
}

Value& State::top() {
    if (m_stack.empty()) {
        throw LuaException("stack underflow");
    }
    
    return m_stack.back();
}

Value& State::get(int index) {
    if (!isValidIndex(index)) {
        throw LuaException("invalid stack index");
    }
    
    int absIndex = absoluteIndex(index);
    return m_stack[absIndex];
}

void State::remove(int index) {
    if (!isValidIndex(index)) {
        throw LuaException("invalid stack index");
    }
    
    int absIndex = absoluteIndex(index);
    m_stack.erase(m_stack.begin() + absIndex);
}

int State::getTop() const {
    return static_cast<int>(m_stack.size());
}

void State::setTop(int index) {
    if (index < 0) {
        // Convert negative index to absolute
        index = static_cast<int>(m_stack.size()) + index + 1;
    }
    
    if (index < 0) {
        throw LuaException("invalid stack index");
    }
    
    // Adjust stack size
    if (static_cast<size_t>(index) > m_stack.size()) {
        // Grow the stack with nil values
        m_stack.resize(index, Value());
    }
    else if (static_cast<size_t>(index) < m_stack.size()) {
        // Shrink the stack
        m_stack.resize(index);
    }
}

// Type checking for stack values
bool State::isNil(int index) const {
    if (!isValidIndex(index)) {
        return true; // Invalid indices are treated as nil
    }
    
    int absIndex = absoluteIndex(index);
    return m_stack[absIndex].isNil();
}

bool State::isBoolean(int index) const {
    if (!isValidIndex(index)) {
        return false;
    }
    
    int absIndex = absoluteIndex(index);
    return m_stack[absIndex].isBoolean();
}

bool State::isNumber(int index) const {
    if (!isValidIndex(index)) {
        return false;
    }
    
    int absIndex = absoluteIndex(index);
    return m_stack[absIndex].isNumber();
}

bool State::isString(int index) const {
    if (!isValidIndex(index)) {
        return false;
    }
    
    int absIndex = absoluteIndex(index);
    return m_stack[absIndex].isString();
}

bool State::isTable(int index) const {
    if (!isValidIndex(index)) {
        return false;
    }
    
    int absIndex = absoluteIndex(index);
    return m_stack[absIndex].isTable();
}

bool State::isFunction(int index) const {
    if (!isValidIndex(index)) {
        return false;
    }
    
    int absIndex = absoluteIndex(index);
    return m_stack[absIndex].isFunction();
}

bool State::isUserData(int index) const {
    if (!isValidIndex(index)) {
        return false;
    }
    
    int absIndex = absoluteIndex(index);
    return m_stack[absIndex].isUserData();
}

// Value retrieval from stack
bool State::toBoolean(int index) const {
    if (!isValidIndex(index)) {
        return false;
    }
    
    int absIndex = absoluteIndex(index);
    const Value& value = m_stack[absIndex];
    
    if (value.isBoolean()) {
        return value.asBoolean();
    }
    
    // Lua truth value rules: nil and false are false, everything else is true
    return !value.isNil();
}

double State::toNumber(int index) const {
    if (!isValidIndex(index)) {
        throw LuaException("invalid stack index");
    }
    
    int absIndex = absoluteIndex(index);
    const Value& value = m_stack[absIndex];
    
    if (value.isNumber()) {
        return value.asNumber();
    }
    
    throw LuaException("value is not a number");
}

std::string State::toString(int index) const {
    if (!isValidIndex(index)) {
        throw LuaException("invalid stack index");
    }
    
    int absIndex = absoluteIndex(index);
    const Value& value = m_stack[absIndex];
    
    if (value.isString()) {
        return value.asString();
    }
    
    throw LuaException("value is not a string");
}

std::shared_ptr<Table> State::toTable(int index) const {
    if (!isValidIndex(index)) {
        throw LuaException("invalid stack index");
    }
    
    int absIndex = absoluteIndex(index);
    const Value& value = m_stack[absIndex];
    
    if (value.isTable()) {
        return value.asTable();
    }
    
    throw LuaException("value is not a table");
}

std::shared_ptr<Function> State::toFunction(int index) const {
    if (!isValidIndex(index)) {
        throw LuaException("invalid stack index");
    }
    
    int absIndex = absoluteIndex(index);
    const Value& value = m_stack[absIndex];
    
    if (value.isFunction()) {
        return value.asFunction();
    }
    
    throw LuaException("value is not a function");
}

std::shared_ptr<UserData> State::toUserData(int index) const {
    if (!isValidIndex(index)) {
        throw LuaException("invalid stack index");
    }
    
    int absIndex = absoluteIndex(index);
    const Value& value = m_stack[absIndex];
    
    if (value.isUserData()) {
        return value.asUserData();
    }
    
    throw LuaException("value is not a userdata");
}

// Global table access
void State::setGlobal(const std::string& name, const Value& value) {
    m_globals->set(Value(name), value);
}

Value State::getGlobal(const std::string& name) const {
    return m_globals->get(Value(name));
}

// Function calls
int State::call(int nargs, int nresults) {
    if (m_stack.size() < static_cast<size_t>(nargs) + 1) {
        throw LuaException("not enough arguments for function call");
    }
    
    // Get function object
    int funcIndex = m_stack.size() - nargs - 1;
    if (!m_stack[funcIndex].isFunction()) {
        throw LuaException("attempt to call a non-function value");
    }
    
    auto func = m_stack[funcIndex].asFunction();
    
    // Call the function (this will be expanded when we implement the VM)
    int ret = func->call(this, nargs, nresults);
    
    return ret;
}

// Check if a stack index is valid
bool State::isValidIndex(int index) const {
    if (index == 0) {
        return false; // 0 is never a valid index
    }
    
    int absIndex = absoluteIndex(index);
    return absIndex >= 0 && absIndex < static_cast<int>(m_stack.size());
}

// Convert relative index to absolute index
int State::absoluteIndex(int index) const {
    if (index > 0) {
        return index - 1; // Convert from 1-based to 0-based
    }
    else if (index < 0) {
        // Negative indices count from the top of the stack
        return static_cast<int>(m_stack.size()) + index;
    }
    else {
        return 0; // index 0 should never happen, handled by isValidIndex
    }
}

} // namespace LuaCore
