#pragma once

#include "types.hpp"
#include "gc/gc_object.hpp"

namespace Lua {

// 前向声明
namespace Object {
    class Function;
    class Value;
}

namespace VM {

// 前向声明
class State;

/**
 * @brief 调用帧类，用于管理函数调用状态
 * 
 * CallInfo类维护函数调用过程中所需的所有状态信息，包括：
 * - 当前执行的函数
 * - 函数参数和局部变量在栈中的位置
 * - 当前指令执行位置
 * - 返回地址
 * - 预期的返回值数量
 */
class CallInfo {
public:
    // 函数调用类型
    enum class CallType {
        Lua,        // Lua函数调用
        C,          // C++函数调用
        Tail        // 尾调用优化
    };

    /**
     * @brief 构造Lua函数调用帧
     * 
     * @param state 虚拟机状态
     * @param func 被调用的函数
     * @param base 栈基址（参数起始位置）
     * @param nargs 参数数量
     * @param nresults 期望的返回值数量
     */
    CallInfo(State* state, Ptr<Object::Function> func, i32 base, i32 nargs, i32 nresults);
    
    /**
     * @brief 构造C++函数调用帧
     * 
     * @param state 虚拟机状态
     * @param func 被调用的C++函数
     * @param base 栈基址（参数起始位置）
     * @param nargs 参数数量
     * @param nresults 期望的返回值数量
     */
    CallInfo(State* state, Ptr<Object::Function> func, i32 base, i32 nargs, i32 nresults, CallType type);
    
    ~CallInfo() = default;
    
    // 获取当前函数
    Ptr<Object::Function> getFunction() const { return m_function; }
    
    // 获取/设置当前指令计数器
    i32 getPC() const { return m_pc; }
    void setPC(i32 pc) { m_pc = pc; }
    void incPC() { m_pc++; }
    
    // 获取/设置栈基址
    i32 getBase() const { return m_base; }
    void setBase(i32 base) { m_base = base; }
    
    // 获取/设置栈顶
    i32 getTop() const { return m_top; }
    void setTop(i32 top) { m_top = top; }
    
    // 获取参数数量和返回值数量
    i32 getNumArgs() const { return m_nargs; }
    i32 getNumResults() const { return m_nresults; }
    
    // 判断调用类型
    bool isLuaCall() const { return m_callType == CallType::Lua; }
    bool isCCall() const { return m_callType == CallType::C; }
    bool isTailCall() const { return m_callType == CallType::Tail; }
    
    // 调用帧链接
    CallInfo* getPrevious() const { return m_previous; }
    void setPrevious(CallInfo* prev) { m_previous = prev; }
    
    CallInfo* getNext() const { return m_next; }
    void setNext(CallInfo* next) { m_next = next; }
    
    // 虚拟机状态引用
    State* getState() const { return m_state; }
    
    // 栈索引相对于当前调用帧的转换
    i32 getAbsoluteIndex(i32 idx) const;
    
    // 获取局部变量
    Object::Value& getLocal(i32 idx);
    
    // 获取上值
    Object::Value& getUpvalue(i32 idx);
    
    // 获取当前调用状态描述（用于调试）
    Str getCallDescription() const;
    
private:
    State* m_state;                  // 虚拟机状态
    Ptr<Object::Function> m_function; // 当前执行的函数
    i32 m_pc;                        // 指令计数器
    i32 m_base;                      // 栈基址（函数参数起始位置）
    i32 m_top;                       // 栈顶（局部变量结束位置）
    i32 m_nargs;                     // 参数数量
    i32 m_nresults;                  // 期望的返回值数量
    CallType m_callType;             // 调用类型
    
    CallInfo* m_previous;            // 上一个调用帧
    CallInfo* m_next;                // 下一个调用帧
};

} // namespace VM
} // namespace Lua
