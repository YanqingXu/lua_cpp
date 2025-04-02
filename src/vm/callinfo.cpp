#include "callinfo.hpp"

namespace Lua {

CallInfo::CallInfo(State* state, Ptr<Function> func, i32 base, i32 nargs, i32 nresults)
    : m_state(state),
      m_function(func),
      m_pc(0),
      m_base(base),
      m_top(base + nargs),
      m_nargs(nargs),
      m_nresults(nresults),
      m_callType(CallType::Lua),
      m_previous(nullptr),
      m_next(nullptr) {
}

CallInfo::CallInfo(State* state, Ptr<Function> func, i32 base, i32 nargs, i32 nresults, CallType type)
    : m_state(state),
      m_function(func),
      m_pc(0),
      m_base(base),
      m_top(base + nargs),
      m_nargs(nargs),
      m_nresults(nresults),
      m_callType(type),
      m_previous(nullptr),
      m_next(nullptr) {
}

i32 CallInfo::getAbsoluteIndex(i32 idx) const {
    // 正数索引相对于base，负数索引相对于top
    if (idx > 0) {
        return m_base + (idx - 1);
    } else if (idx < 0) {
        return m_top + idx;
    } else {
        // idx == 0 是无效的索引
        throw std::runtime_error("Invalid stack index 0");
    }
}

Value& CallInfo::getLocal(i32 idx) {
    i32 absIdx = getAbsoluteIndex(idx);
    
    // 检查索引是否在有效范围内
    if (absIdx < m_base || absIdx >= m_top) {
        throw std::runtime_error("Local index out of range");
    }
    
    return m_state->getStack()[absIdx];
}

Value& CallInfo::getUpvalue(i32 idx) {
    // 上值访问需要通过函数的闭包环境
    // 此处需要VM::State提供上值访问的接口
    // 目前简单抛出异常，等待后续实现
    throw std::runtime_error("Upvalue access not implemented yet");
}

Str CallInfo::getCallDescription() const {
    std::ostringstream oss;
    
    // 获取函数信息
    if (m_function) {
        if (m_function->isNative()) {
            oss << "C function";
        } else {
            // 如果有函数名，显示函数名
            oss << "Lua function";
            // TODO: 从FunctionProto获取函数名
        }
    } else {
        oss << "unknown function";
    }
    
    // 添加调用位置信息
    if (isLuaCall() && m_pc > 0) {
        oss << " at line ";
        // TODO: 从调试信息获取行号
        oss << "unknown";
    }
    
    return oss.str();
}
} // namespace Lua
