#pragma once

#include <memory>

// 前向声明
class GarbageCollector;

namespace Lua {
/**
 * @brief Lua 中所有可垃圾回收对象的基类
 * 
 * GCObject 作为所有由垃圾收集器管理的 Lua 对象的基类，
 * 包括表、函数和用户数据等。它提供了垃圾收集标记和
 * 生命周期管理的通用功能。
 */
class GCObject : public std::enable_shared_from_this<GCObject> {
public:
	// 虚析构函数确保派生类的正确清理
    virtual ~GCObject() = default;
    
    // 垃圾收集支持
    virtual void mark(GarbageCollector* gc) { m_marked = true; }
    bool isMarked() const { return m_marked; }
    void unmark() { m_marked = false; }
    
    // 对象类型枚举
    enum class Type {
        String,
        Table,
        Closure,
        UserData,
        Function,
        Thread
    };
    
    // 获取 GCObject 的类型（由派生类实现）
    virtual Type type() const = 0;
    
protected:
	// 受保护的构造函数，防止直接实例化 GCObject
    GCObject() : m_marked(false) {}
    
private:
	// 标记标志，用于垃圾回收
    bool m_marked;
};

} // namespace Lua
