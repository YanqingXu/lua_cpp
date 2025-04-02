#pragma once

#include "types.hpp"
#include "gc/gc_object.hpp"
#include "vm/callinfo.hpp"

class State; // 前向声明

namespace Lua {

/**
 * @brief Lua线程/协程类
 * 
 * Thread类表示Lua中的一个线程或协程。在Lua中，协程是轻量级的线程，
 * 允许在同一程序内多个执行路径交替执行。每个协程有自己的栈和执行状态，
 * 但共享全局变量和大多数对象。
 */
class Thread : public GCObject {
public:
    // 线程状态枚举
    enum class Status {
        Ready,      // 准备就绪，可以运行或恢复
        Running,    // 正在运行
        Suspended,  // 已挂起，可以恢复
        Normal,     // 已正常终止
        Error,      // 错误终止
        Dead        // 已死亡，不可恢复
    };

    /**
     * @brief 构造函数
     * 
     * @param parent 父状态/主线程
     */
    explicit Thread(VM::State* parent);
    
    /**
     * @brief 析构函数
     */
    ~Thread() override;
    
    /**
     * @brief 获取线程状态
     */
    Status getStatus() const { return m_status; }
    
    /**
     * @brief 设置线程状态
     */
    void setStatus(Status status) { m_status = status; }
    
    /**
     * @brief 恢复线程执行
     * 
     * @param nargs 参数数量
     * @return 返回值数量，或负数表示错误
     */
    i32 resume(i32 nargs);
    
    /**
     * @brief 挂起线程执行
     * 
     * @param nresults 返回结果数量
     * @return 挂起时需要的返回值数量
     */
    i32 yield(i32 nresults);
    
    /**
     * @brief 获取线程栈大小
     */
    i32 getStackSize() const { return m_stackSize; }
    
    /**
     * @brief 获取当前调用信息
     */
    VM::CallInfo* getCurrentCallInfo() const { return m_currentCallInfo; }
    
    /**
     * @brief 设置当前调用信息
     */
    void setCurrentCallInfo(VM::CallInfo* ci) { m_currentCallInfo = ci; }
    
    /**
     * @brief GC标记方法实现
     */
    void mark(GarbageCollector* gc) override;
    
    /**
     * @brief 获取父状态/主线程
     */
    VM::State* getParentState() const { return m_parent; }
    
    /**
     * @brief 获取对象类型
     */
    Type type() const override { return Type::Thread; }

private:
    VM::State* m_parent;                // 父状态/主线程
    Status m_status;                    // 线程状态
    i32 m_stackSize;                    // 栈大小
    VM::CallInfo* m_currentCallInfo;    // 当前调用信息
};

} // namespace Lua
