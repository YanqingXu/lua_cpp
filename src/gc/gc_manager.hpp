#pragma once

#include "gc_object.hpp"
#include <vector>
#include <memory>

namespace GC {

/**
 * @brief 垃圾收集器管理器类
 * 
 * GCManager 管理 Lua 运行时中的垃圾收集过程。
 * 它负责跟踪、标记和清除不再需要的对象。
 */
class GCManager {
public:
    // 构造函数
    GCManager() = default;
    
    // 析构函数
    ~GCManager() = default;
    
    // 添加对象到管理器
    void addObject(std::shared_ptr<GCObject> obj);
    
    // 标记所有可到达的对象
    void markReachable();
    
    // 清除未标记的对象
    void sweep();
    
    // 执行完整的垃圾收集周期
    void collect();
    
private:
    // 管理的对象列表
    std::vector<std::weak_ptr<GCObject>> m_objects;
};

} // namespace GC
