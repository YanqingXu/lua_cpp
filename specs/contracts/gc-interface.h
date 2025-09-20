/**
 * @file gc-interface.h
 * @brief 垃圾回收器接口契约
 * @date 2025-09-20
 */

#ifndef GC_INTERFACE_H
#define GC_INTERFACE_H

#include <memory>
#include <vector>
#include <functional>
#include <atomic>

namespace lua_gc {

/* ========================================================================== */
/* 前向声明 */
/* ========================================================================== */

class GCObject;
class GCStats;

/* ========================================================================== */
/* 垃圾回收对象基类 */
/* ========================================================================== */

/**
 * @brief 垃圾回收对象基类
 * 所有需要垃圾回收的对象都应该继承此类
 */
class GCObject {
protected:
    mutable std::atomic<uint8_t> gc_flags_{0};
    
public:
    // GC标记位
    static constexpr uint8_t MARK_BIT = 0x01;     ///< 标记位
    static constexpr uint8_t WEAK_BIT = 0x02;     ///< 弱引用位
    static constexpr uint8_t FIXED_BIT = 0x04;    ///< 固定位（不回收）
    static constexpr uint8_t GRAY_BIT = 0x08;     ///< 灰色位（待处理）
    
    virtual ~GCObject() = default;
    
    /**
     * @brief 标记对象及其引用的所有对象
     * 垃圾回收器在标记阶段调用此方法
     */
    virtual void mark() = 0;
    
    /**
     * @brief 获取对象占用的内存大小
     * @return 内存大小（字节）
     */
    virtual size_t memory_size() const = 0;
    
    /**
     * @brief 获取对象类型名称（用于调试）
     * @return 类型名称
     */
    virtual const char* type_name() const = 0;
    
    // GC标记操作
    bool is_marked() const { return (gc_flags_.load() & MARK_BIT) != 0; }
    void set_marked(bool marked) {
        if (marked) {
            gc_flags_.fetch_or(MARK_BIT);
        } else {
            gc_flags_.fetch_and(~MARK_BIT);
        }
    }
    
    bool is_weak() const { return (gc_flags_.load() & WEAK_BIT) != 0; }
    void set_weak(bool weak) {
        if (weak) {
            gc_flags_.fetch_or(WEAK_BIT);
        } else {
            gc_flags_.fetch_and(~WEAK_BIT);
        }
    }
    
    bool is_fixed() const { return (gc_flags_.load() & FIXED_BIT) != 0; }
    void set_fixed(bool fixed) {
        if (fixed) {
            gc_flags_.fetch_or(FIXED_BIT);
        } else {
            gc_flags_.fetch_and(~FIXED_BIT);
        }
    }
    
    bool is_gray() const { return (gc_flags_.load() & GRAY_BIT) != 0; }
    void set_gray(bool gray) {
        if (gray) {
            gc_flags_.fetch_or(GRAY_BIT);
        } else {
            gc_flags_.fetch_and(~GRAY_BIT);
        }
    }
};

/* ========================================================================== */
/* 垃圾回收器接口 */
/* ========================================================================== */

/**
 * @brief 垃圾回收器状态
 */
enum class GCState {
    IDLE,           ///< 空闲状态
    MARKING,        ///< 标记阶段
    SWEEPING,       ///< 清扫阶段
    FINALIZING      ///< 析构阶段
};

/**
 * @brief 垃圾回收器接口
 */
class IGarbageCollector {
public:
    virtual ~IGarbageCollector() = default;
    
    /**
     * @brief 注册GC对象
     * @param obj 要注册的对象
     */
    virtual void register_object(std::shared_ptr<GCObject> obj) = 0;
    
    /**
     * @brief 注销GC对象
     * @param obj 要注销的对象
     */
    virtual void unregister_object(std::shared_ptr<GCObject> obj) = 0;
    
    /**
     * @brief 执行完整的垃圾回收
     * @return 回收的内存字节数
     */
    virtual size_t collect() = 0;
    
    /**
     * @brief 执行增量垃圾回收步骤
     * @param step_size 步骤大小（处理对象数或时间限制）
     * @return 是否完成了一个完整的回收周期
     */
    virtual bool step(size_t step_size) = 0;
    
    /**
     * @brief 停止垃圾回收
     */
    virtual void stop() = 0;
    
    /**
     * @brief 重启垃圾回收
     */
    virtual void restart() = 0;
    
    /**
     * @brief 检查是否应该触发垃圾回收
     * @return 是否应该执行GC
     */
    virtual bool should_collect() const = 0;
    
    /**
     * @brief 获取当前GC状态
     * @return GC状态
     */
    virtual GCState get_state() const = 0;
    
    /**
     * @brief 设置GC参数
     * @param pause GC暂停倍数（百分比）
     * @param step_multiplier 步进倍数
     */
    virtual void set_parameters(int pause, int step_multiplier) = 0;
    
    /**
     * @brief 获取GC统计信息
     * @return GC统计
     */
    virtual GCStats get_stats() const = 0;
    
    /**
     * @brief 添加根对象
     * @param obj 根对象（永远不会被回收）
     */
    virtual void add_root(std::shared_ptr<GCObject> obj) = 0;
    
    /**
     * @brief 移除根对象
     * @param obj 要移除的根对象
     */
    virtual void remove_root(std::shared_ptr<GCObject> obj) = 0;
};

/* ========================================================================== */
/* 弱引用管理器接口 */
/* ========================================================================== */

/**
 * @brief 弱引用表类型
 */
enum class WeakTableType {
    WEAK_KEYS,      ///< 弱键表
    WEAK_VALUES,    ///< 弱值表
    WEAK_BOTH       ///< 键值都弱的表
};

/**
 * @brief 弱引用管理器接口
 */
class IWeakReferenceManager {
public:
    virtual ~IWeakReferenceManager() = default;
    
    /**
     * @brief 注册弱引用表
     * @param table 弱引用表
     * @param type 弱引用类型
     */
    virtual void register_weak_table(std::shared_ptr<GCObject> table,
                                    WeakTableType type) = 0;
    
    /**
     * @brief 注销弱引用表
     * @param table 要注销的弱引用表
     */
    virtual void unregister_weak_table(std::shared_ptr<GCObject> table) = 0;
    
    /**
     * @brief 清理所有弱引用表中的无效引用
     */
    virtual void cleanup_weak_references() = 0;
    
    /**
     * @brief 检查对象是否在弱引用表中
     * @param obj 要检查的对象
     * @return 是否在弱引用表中
     */
    virtual bool is_in_weak_table(std::shared_ptr<GCObject> obj) const = 0;
};

/* ========================================================================== */
/* 析构器管理器接口 */
/* ========================================================================== */

/**
 * @brief 析构器回调函数类型
 */
using FinalizerCallback = std::function<void(std::shared_ptr<GCObject>)>;

/**
 * @brief 析构器管理器接口
 */
class IFinalizerManager {
public:
    virtual ~IFinalizerManager() = default;
    
    /**
     * @brief 注册析构器
     * @param obj 对象
     * @param finalizer 析构器回调
     */
    virtual void register_finalizer(std::shared_ptr<GCObject> obj,
                                   FinalizerCallback finalizer) = 0;
    
    /**
     * @brief 注销析构器
     * @param obj 对象
     */
    virtual void unregister_finalizer(std::shared_ptr<GCObject> obj) = 0;
    
    /**
     * @brief 运行所有待执行的析构器
     */
    virtual void run_finalizers() = 0;
    
    /**
     * @brief 检查是否有待执行的析构器
     * @return 是否有待执行的析构器
     */
    virtual bool has_pending_finalizers() const = 0;
};

/* ========================================================================== */
/* 内存分配器接口 */
/* ========================================================================== */

/**
 * @brief 内存分配器接口
 */
class IMemoryAllocator {
public:
    virtual ~IMemoryAllocator() = default;
    
    /**
     * @brief 分配内存
     * @param size 要分配的字节数
     * @return 分配的内存指针，失败返回nullptr
     */
    virtual void* allocate(size_t size) = 0;
    
    /**
     * @brief 释放内存
     * @param ptr 要释放的内存指针
     * @param size 内存大小
     */
    virtual void deallocate(void* ptr, size_t size) = 0;
    
    /**
     * @brief 重新分配内存
     * @param ptr 原内存指针
     * @param old_size 原内存大小
     * @param new_size 新内存大小
     * @return 新内存指针
     */
    virtual void* reallocate(void* ptr, size_t old_size, size_t new_size) = 0;
    
    /**
     * @brief 获取已分配的总内存大小
     * @return 已分配内存字节数
     */
    virtual size_t get_allocated_size() const = 0;
    
    /**
     * @brief 获取分配次数统计
     * @return 分配次数
     */
    virtual size_t get_allocation_count() const = 0;
};

/* ========================================================================== */
/* GC统计信息 */
/* ========================================================================== */

/**
 * @brief 垃圾回收统计信息
 */
struct GCStats {
    size_t total_memory;          ///< 总内存使用量
    size_t gc_memory;             ///< GC管理的内存
    size_t num_objects;           ///< 对象总数
    size_t num_collections;       ///< 回收次数
    size_t total_collected;       ///< 总回收字节数
    double last_collection_time;  ///< 上次回收耗时（毫秒）
    double total_collection_time; ///< 总回收耗时（毫秒）
    size_t num_finalizers;        ///< 待执行析构器数量
    size_t num_weak_tables;       ///< 弱引用表数量
    
    // 分代信息（如果支持）
    size_t young_generation_size; ///< 年轻代大小
    size_t old_generation_size;   ///< 老年代大小
    size_t young_collections;     ///< 年轻代回收次数
    size_t old_collections;       ///< 老年代回收次数
};

/* ========================================================================== */
/* 写屏障接口 */
/* ========================================================================== */

/**
 * @brief 写屏障接口（用于增量/分代GC）
 */
class IWriteBarrier {
public:
    virtual ~IWriteBarrier() = default;
    
    /**
     * @brief 对象写入屏障
     * @param parent 父对象
     * @param child 子对象
     */
    virtual void object_write(std::shared_ptr<GCObject> parent,
                             std::shared_ptr<GCObject> child) = 0;
    
    /**
     * @brief 表写入屏障
     * @param table 表对象
     * @param key 键
     * @param value 值
     */
    virtual void table_write(std::shared_ptr<GCObject> table,
                            std::shared_ptr<GCObject> key,
                            std::shared_ptr<GCObject> value) = 0;
    
    /**
     * @brief 启用写屏障
     */
    virtual void enable() = 0;
    
    /**
     * @brief 禁用写屏障
     */
    virtual void disable() = 0;
    
    /**
     * @brief 检查写屏障是否启用
     * @return 是否启用
     */
    virtual bool is_enabled() const = 0;
};

} // namespace lua_gc

#endif /* GC_INTERFACE_H */