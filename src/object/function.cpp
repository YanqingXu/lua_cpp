#include "function.hpp"
#include "value.hpp"
#include "vm/state.hpp"
#include "vm/function_proto.hpp"

namespace Lua {
namespace Object {

// Function实现
Function::Function(Ptr<FunctionProto> proto)
    : GC::GCObject(GC::GCObject::Type::Function),
      m_prototype(proto), 
      m_cFunction(nullptr), 
      m_isNative(false) {
}

Function::Function(std::function<i32(State*)> func)
    : GC::GCObject(GC::GCObject::Type::Function),
      m_prototype(nullptr), 
      m_cFunction(func), 
      m_isNative(true) {
}

i32 Function::call(State* state, i32 nargs, i32 nresults) {
    if (isNative()) {
        // 直接调用C++函数
        return m_cFunction(state);
    }
    else {
        // Lua函数 - 这部分将由VM实现
        // 目前仅返回错误
        throw std::runtime_error("Lua函数执行尚未实现");
    }
}

void Function::mark() {
    if (isMarked()) {
        return;
    }
    
    GC::GCObject::mark();
    
    // 如果是Lua函数，标记函数原型
    if (!isNative() && m_prototype) {
        // 原型本身不是GCObject，但包含GC对象的引用
        // 这里应该根据FunctionProto的实际实现方式标记其中的GC对象
        // TODO: 待FunctionProto实现后完善此处
    }
    
    // 标记上值
    for (auto& upvalue : m_upvalues) {
        if (upvalue) {
            upvalue->mark();
        }
    }
}

} // namespace Object
} // namespace Lua
