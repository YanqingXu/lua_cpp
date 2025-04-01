#pragma once

#include "gc_object.hpp"
#include "types.hpp"

namespace Lua {

// 前向声明
namespace Object {
    class String;
    class Table;
    class Function;
    class UserData;
}

namespace VM {
    class State;
    class FunctionProto;
}

namespace GC {

/**
 * @brief 字符串池，用于内部化字符串对象
 * 
 * StringPool负责管理Lua中的字符串对象，确保相同的字符串只有一个实例，
 * 以节省内存并提高字符串比较性能。
 */
class StringPool {
public:
    StringPool();
    ~StringPool() = default;
    
    // 查找字符串，如果存在则返回，否则返回nullptr
    Ptr<Object::String> find(const Str& value);
    
    // 添加一个字符串到池中
    void add(const Ptr<Object::String>& str);
    
    // 从池中移除一个字符串
    void remove(const Ptr<Object::String>& str);
    
    // 清空字符串池
    void clear();
    
private:
    // 计算字符串哈希值
    u32 computeHash(const Str& value);
    
    // 字符串映射表，使用哈希值作为键
    HashMap<u32, Vec<Ptr<Object::String>>> m_strings;
};

/**
 * @brief 管理Lua对象的内存分配和垃圾回收
 * 
 * GarbageCollector负责跟踪和回收Lua对象使用的内存。它实现了一个标记-清除
 * 垃圾回收算法，具有增量收集功能。
 */
class GarbageCollector {
public:
    // 构造函数
    GarbageCollector();
    
    // 析构函数确保清理
    ~GarbageCollector();
    
    // 创建基本Lua对象
    Ptr<Object::String> createString(const Str& value);
    Ptr<Object::Table> createTable(i32 narray = 0, i32 nrec = 0);
    Ptr<Object::Function> createFunction(Ptr<VM::FunctionProto> proto);
    Ptr<Object::UserData> createUserData(usize size);
    
    // 执行一个完整的垃圾回收周期
    void collectGarbage();
    
    // 执行增量垃圾回收步骤
    void collectGarbageIncremental();
    
    // 强制执行完整的垃圾回收
    void fullGC();
    
    // 暂停和恢复垃圾回收
    void pauseGC();
    void resumeGC();
    
    // 获取当前内存使用量
    usize getTotalMemory() const { return m_totalMemory; }
    
    // 设置GC阈值
    void setThreshold(usize threshold) { m_gcThreshold = threshold; }
    usize getThreshold() const { return m_gcThreshold; }
    
    // 访问字符串池
    StringPool& getStringPool() { return *m_stringPool; }
    
    // 状态引用，用于设置Lua状态
    VM::State* m_state;
    
private:
    // 从Lua状态标记根对象
    void markRoots();
    
    // 标记阶段 - 标记所有可达对象
    void mark();
    
    // 清除阶段 - 回收不可达对象的内存
    void sweep();
    
    // 检查是否应该触发GC
    void maybeGC();
    
    // 获取对象大小的辅助函数
    usize getObjectSize(const Ptr<GCObject>& obj) const;
    
    // GC常量
    static constexpr usize GC_INITIAL_THRESHOLD = 1024 * 1024; // 1MB
    static constexpr double GC_THRESHOLD_FACTOR = 2.0;         // 回收后阈值增加因子
    
    // 字符串池
    Ptr<StringPool> m_stringPool;
    
    // 所有GC管理的对象
    Vec<Ptr<GCObject>> m_objects;
    
    // 内存使用情况
    usize m_totalMemory;         // 当前内存使用量
    usize m_gcThreshold;         // 触发GC的内存阈值
    
    // GC控制
    bool m_pause;                // 是否暂停GC
    
    // 增量GC状态
    enum class GCState {
        Idle,                    // 无GC进行中
        Mark,                    // 标记阶段进行中
        Sweep                    // 清除阶段进行中
    };
    
    GCState m_gcState;           // 当前增量GC状态
    Vec<Ptr<GCObject>> m_toMark;  // 待标记对象队列
    usize m_currentIndex;       // 增量GC当前索引
    
    // 友元类
    friend class VM::State;
};

} // namespace GC
} // namespace Lua
