#include "state.hpp"
#include "config.hpp"
#include "object/string.hpp"
#include "object/table.hpp"
#include "object/function.hpp"

namespace Lua {
namespace VM {

// 创建一个新的Lua状态
std::shared_ptr<State> State::create() {
    auto state = std::shared_ptr<State>(new State());
    state->initialize();
    return state;
}

// 构造函数
State::State() : m_stackTop(0), m_callDepth(0) {
    // 初始化堆栈
    m_stack.resize(LUA_MINSTACK);
    
    // 初始化垃圾收集器
    m_gc = std::make_unique<GC::GarbageCollector>();
    m_gc->m_state = this;
}

// 析构函数
State::~State() {
    // 清理所有对象
    m_stack.clear();
    m_globals.reset();
    m_registry.reset();
    m_gc.reset();
}

// 初始化状态
void State::initialize() {
    // 创建全局环境
    m_globals = std::make_shared<Object::Table>();
    m_registry = std::make_shared<Object::Table>();
    
    // TODO: 注册基本函数到全局环境
}

// 打开标准库
void State::openLibs() {
    // TODO: 实现标准库加载
}

// 执行Lua代码
i32 State::doString(const Str& code) {
    // TODO: 实现代码执行
    return 0;
}

// 执行Lua文件
i32 State::doFile(const Str& filename) {
    // TODO: 实现文件加载和执行
    return 0;
}

// 堆栈操作
void State::push(const Object::Value& value) {
    // 确保堆栈有足够空间
    checkStack(1);
    m_stack[m_stackTop++] = value;
}

void State::pushNil() {
    push(Object::Value::nil());
}

void State::pushBoolean(bool b) {
    push(Object::Value::boolean(b));
}

void State::pushNumber(double n) {
    push(Object::Value::number(n));
}

void State::pushString(const Str& s) {
    auto strObj = m_gc->createString(s);
    push(Object::Value::string(strObj));
}

void State::pushTable(Ptr<Object::Table> table) {
    push(Object::Value::table(table));
}

void State::pushFunction(Ptr<Object::Function> function) {
    push(Object::Value::function(function));
}

Object::Value State::pop() {
    if (m_stackTop <= 0) {
        throw LuaException("Stack underflow");
    }
    return m_stack[--m_stackTop];
}

void State::pop(i32 n) {
    if (n > m_stackTop) {
        throw LuaException("Cannot pop more elements than exist on stack");
    }
    m_stackTop -= n;
}

Object::Value State::peek(i32 index) const {
    i32 absIdx = absIndex(index);
    if (absIdx <= 0 || absIdx > m_stackTop) {
        return Object::Value::nil();
    }
    return m_stack[absIdx - 1];
}

bool State::checkStack(i32 n) {
    if (m_stackTop + n > static_cast<i32>(m_stack.size())) {
        // 需要扩展堆栈
        m_stack.resize(m_stack.size() * 2);
    }
    return true;
}

i32 State::absIndex(i32 index) const {
    if (index > 0) {
        return index;
    } else if (index <= LUA_REGISTRYINDEX) {
        // 特殊索引
        return index;
    } else {
        return m_stackTop + index + 1;
    }
}

i32 State::getTop() const {
    return m_stackTop;
}

void State::setTop(i32 index) {
    i32 absIdx = absIndex(index);
    if (absIdx < 0) {
        throw LuaException("Invalid stack index");
    }
    
    if (absIdx > m_stackTop) {
        // 堆栈扩展，新位置填充nil
        checkStack(absIdx - m_stackTop);
        for (i32 i = m_stackTop; i < absIdx; ++i) {
            m_stack[i] = Object::Value::nil();
        }
    }
    
    m_stackTop = absIdx;
}

// 表操作
void State::createTable(i32 narray, i32 nrec) {
    auto table = m_gc->createTable(narray, nrec);
    push(Object::Value::table(table));
}

void State::getTable(i32 index) {
    Object::Value t = peek(index);
    Object::Value k = pop();
    
    if (t.isTable()) {
        auto table = t.asTable();
        push(table->get(k));
    } else {
        throw LuaException("Not a table");
    }
}

void State::setTable(i32 index) {
    Object::Value t = peek(index);
    Object::Value k = peek(-2);
    Object::Value v = peek(-1);
    
    if (t.isTable()) {
        auto table = t.asTable();
        table->set(k, v);
        pop(2); // 弹出键和值
    } else {
        throw LuaException("Not a table");
    }
}

void State::getField(i32 index, const Str& k) {
    Object::Value t = peek(index);
    
    if (t.isTable()) {
        auto table = t.asTable();
        push(table->get(Object::Value::string(m_gc->createString(k))));
    } else {
        throw LuaException("Not a table");
    }
}

void State::setField(i32 index, const Str& k) {
    Object::Value t = peek(index);
    Object::Value v = pop();
    
    if (t.isTable()) {
        auto table = t.asTable();
        table->set(Object::Value::string(m_gc->createString(k)), v);
    } else {
        throw LuaException("Not a table");
    }
}

void State::rawGetI(i32 index, i32 i) {
    Object::Value t = peek(index);
    
    if (t.isTable()) {
        auto table = t.asTable();
        push(table->rawGetI(i));
    } else {
        throw LuaException("Not a table");
    }
}

void State::rawSetI(i32 index, i32 i) {
    Object::Value t = peek(index);
    Object::Value v = pop();
    
    if (t.isTable()) {
        auto table = t.asTable();
        table->rawSetI(i, v);
    } else {
        throw LuaException("Not a table");
    }
}

// 全局变量操作
void State::getGlobal(const Str& name) {
    getField(LUA_REGISTRYINDEX, "_G");
    getField(-1, name);
    // 移除全局表
    Object::Value v = peek(-1);
    remove(-2);
    // 只留下获取的值
    m_stack[m_stackTop - 1] = v;
}

void State::setGlobal(const Str& name) {
    getField(LUA_REGISTRYINDEX, "_G");
    Object::Value v = peek(-2);
    setField(-1, name);
    // 移除全局表和原始值
    pop(2);
}

// 类型判断
bool State::isNil(i32 index) const {
    Object::Value v = peek(index);
    return v.isNil();
}

bool State::isBoolean(i32 index) const {
    Object::Value v = peek(index);
    return v.isBoolean();
}

bool State::isNumber(i32 index) const {
    Object::Value v = peek(index);
    return v.isNumber();
}

bool State::isString(i32 index) const {
    Object::Value v = peek(index);
    return v.isString();
}

bool State::isTable(i32 index) const {
    Object::Value v = peek(index);
    return v.isTable();
}

bool State::isFunction(i32 index) const {
    Object::Value v = peek(index);
    return v.isFunction();
}

bool State::isUserData(i32 index) const {
    Object::Value v = peek(index);
    return v.isUserData();
}

bool State::isThread(i32 index) const {
    Object::Value v = peek(index);
    return v.isThread();
}

// 类型转换
bool State::toBoolean(i32 index) const {
    Object::Value v = peek(index);
    return v.toBoolean();
}

double State::toNumber(i32 index) const {
    Object::Value v = peek(index);
    return v.toNumber();
}

Str State::toString(i32 index) const {
    Object::Value v = peek(index);
    if (!v.isString()) {
        throw LuaException("Value is not a string");
    }
    return v.asString()->toString();
}

Ptr<Object::Table> State::toTable(i32 index) const {
    Object::Value v = peek(index);
    if (!v.isTable()) {
        throw LuaException("Value is not a table");
    }
    return v.asTable();
}

Ptr<Object::Function> State::toFunction(i32 index) const {
    Object::Value v = peek(index);
    if (!v.isFunction()) {
        throw LuaException("Value is not a function");
    }
    return v.asFunction();
}

Ptr<Object::UserData> State::toUserData(i32 index) const {
    Object::Value v = peek(index);
    if (!v.isUserData()) {
        throw LuaException("Value is not a userdata");
    }
    return v.asUserData();
}

// 函数调用
i32 State::call(i32 nargs, i32 nresults) {
    // TODO: 实现函数调用
    return 0;
}

// 注册C++函数
void State::registerFunction(const Str& name, CFunction func) {
    // TODO: 创建C函数对象并注册到全局环境
}

// 错误处理
void State::error(const Str& message) {
    throw LuaException(message);
}

} // namespace VM
} // namespace Lua
