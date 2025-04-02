#include "thread.hpp"
#include "value.hpp"
#include "vm/state.hpp"
#include "vm/callinfo.hpp"

namespace Lua {

Thread::Thread(VM::State* parent)
    : m_parent(parent), 
      m_status(Status::Ready), 
      m_stackSize(0),
      m_currentCallInfo(nullptr) {
    // 注意：在实际使用时，我们需要为线程分配自己的栈空间
    // 这里的实现是简化的版本
}

Thread::~Thread() {
    // 清理调用信息链表
    // 注意：在完整实现中，这里应该清理所有与线程相关的资源
}

i32 Thread::resume(i32 nargs) {
    // 检查线程是否可以恢复
    if (m_status != Status::Ready && m_status != Status::Suspended) {
        // 不能恢复非就绪或挂起状态的线程
        return -1;
    }
    
    // 设置线程状态为运行中
    Status oldStatus = m_status;
    m_status = Status::Running;
    
    try {
        // 在实际实现中，这里应该保存当前线程状态，切换到该线程继续执行
        // 由于这是简化版本，我们只做状态转换
        
        // 模拟执行成功
        m_status = Status::Suspended;
        return 1; // 假设有一个返回值
    }
    catch (...) {
        // 发生错误，设置为错误状态
        m_status = Status::Error;
        return -1;
    }
}

i32 Thread::yield(i32 nresults) {
    // 检查线程是否正在运行
    if (m_status != Status::Running) {
        // 只有运行中的线程才能被挂起
        return -1;
    }
    
    // 设置线程状态为挂起
    m_status = Status::Suspended;
    
    // 在实际实现中，这里应该保存当前线程状态，切换回父线程继续执行
    // 由于这是简化版本，我们只做状态转换
    
    return nresults; // 返回nresults表示协程挂起时提供了多少个返回值
}

void Thread::mark(GarbageCollector* gc) {
    // 标记GC过程中，需要标记线程中的所有对象
    if (isMarked()) {
        return; // 已经被标记过了
    }
    
    GCObject::mark(gc); // 调用基类的标记方法
    
    // 在完整实现中，需要标记线程栈中的所有值
    // 以及与线程相关的所有对象引用
}

} // namespace Lua
