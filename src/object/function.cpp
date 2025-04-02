#include "function.hpp"
#include "value.hpp"
#include "vm/state.hpp"
#include "vm/function_proto.hpp"

namespace Lua {

// Function实现
Function::Function(Ptr<FunctionProto> proto)
    : GCObject(), 
      m_prototype(proto), 
      m_cFunction(nullptr), 
      m_isNative(false) {
}

Function::Function(std::function<i32(State*)> func)
    : GCObject(), 
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

void Function::mark(GarbageCollector* gc) {
    // 如果已经被标记，直接返回，避免循环引用导致的无限递归
    if (isMarked()) {
        return;
    }
    
    // 首先标记自身
    GCObject::mark(gc);
    
    // 处理非原生函数的函数原型
    if (!isNative() && m_prototype) {
        // 如果 FunctionProto 继承自 GCObject，应该这样标记
        // m_prototype->mark(gc);
        
        // 如果 FunctionProto 不是 GCObject，但含有 GC 对象引用，
        // 则需要单独处理其中的引用，例如常量表、代码块等
        // 以下是伪代码，需要根据 FunctionProto 的实际实现进行调整
        /*
        // 标记常量表中的GC对象
        for (const auto& constant : m_prototype->constants()) {
            if (constant.isGCObject()) {
                constant.asGCObject()->mark(gc);
            }
        }
        
        // 标记嵌套函数
        for (const auto& nestedFunc : m_prototype->nestedFunctions()) {
            if (nestedFunc) {
                nestedFunc->mark(gc);
            }
        }
        
        // 标记调试信息中的字符串
        if (m_prototype->hasDebugInfo()) {
            for (const auto& debugString : m_prototype->debugStrings()) {
                if (debugString) {
                    debugString->mark(gc);
                }
            }
        }
        */
    }
    
    // 标记所有上值引用
    for (auto& upvalue : m_upvalues) {
        if (upvalue) {
            // 对于 Value 类型的上值，调用其 mark 方法
            upvalue->mark(gc);
        }
    }
}
} // namespace Lua