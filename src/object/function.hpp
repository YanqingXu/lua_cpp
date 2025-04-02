#pragma once

#include "types.hpp"
#include "gc/gc_object.hpp"

// Forward declarations
class State;
class FunctionProto;
class Value;


namespace Lua {

/**
 * @brief Represents a function in Lua
 * 
 * The Function class is the base for all function objects that can
 * be called from Lua code. It provides the common interface for function calls.
 */
class Function : public GCObject {
public:
    // 构造函数
    explicit Function(Ptr<FunctionProto> proto);
    explicit Function(std::function<i32(State*)> func);
    virtual ~Function() = default;
    
    // 函数调用接口
    i32 call(State* state, i32 nargs, i32 nresults);
    
    // 垃圾回收接口
    virtual void mark(GarbageCollector* gc);
    
    // 类型判断
    GCObject::Type type() const override { return Type::Function; }
    bool isNative() const { return m_isNative; }
    
    // 获取函数原型
    Ptr<FunctionProto> prototype() const { return m_prototype; }
    
    // 兼容方法，等同于 prototype()
    Ptr<FunctionProto> getProto() const { return m_prototype; }
    
private:
    Ptr<FunctionProto> m_prototype;           // 函数原型（对于Lua函数）
    std::function<i32(State*)> m_cFunction;   // C++函数（对于本地函数）
    Vec<Ptr<Value>> m_upvalues;                   // 上值（捕获的变量）
    bool m_isNative;                              // 是否为本地C++函数
};
} // namespace Lua
