#pragma once

#include "types.hpp"
#include "gc/gc_object.hpp"


namespace Lua {

// Forward declarations
namespace VM {
    class State;
    class FunctionProto;
}

namespace Object {

// Forward declarations
class Value;

/**
 * @brief Represents a function in Lua
 * 
 * The Function class is the base for all function objects that can
 * be called from Lua code. It provides the common interface for function calls.
 */
class Function : public GC::GCObject {
public:
    // 构造函数
    explicit Function(Ptr<VM::FunctionProto> proto);
    explicit Function(std::function<i32(VM::State*)> func);
    virtual ~Function() = default;
    
    // 函数调用接口
    i32 call(VM::State* state, i32 nargs, i32 nresults);
    
    // 垃圾回收接口
    void mark() override;
    
    // 类型判断
    GC::GCObject::Type type() const override { return Type::Function; }
    bool isNative() const { return m_isNative; }
    
    // 获取函数原型
    Ptr<VM::FunctionProto> prototype() const { return m_prototype; }
    
private:
    Ptr<VM::FunctionProto> m_prototype;           // 函数原型（对于Lua函数）
    std::function<i32(VM::State*)> m_cFunction;   // C++函数（对于本地函数）
    Vec<Ptr<Value>> m_upvalues;                   // 上值（捕获的变量）
    bool m_isNative;                              // 是否为本地C++函数
};

} // namespace Object
} // namespace Lua
