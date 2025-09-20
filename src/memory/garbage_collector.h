#pragma once

#include "core/common.h"
#include "core/lua_value.h"
#include "core/error.h"
#include <memory>
#include <vector>
#include <functional>
#include <atomic>
#include <chrono>
#include <unordered_set>

namespace lua_cpp {

/* ========================================================================== */
/* 前向声明 */
/* ========================================================================== */

class VirtualMachine;
class LuaTable;

/* ========================================================================== */
/* GC错误类型 */
/* ========================================================================== */

/**
 * @brief 内存不足错误
 */
class OutOfMemoryError : public LuaError {
public:
    explicit OutOfMemoryError(const std::string& message = "Out of memory")
        : LuaError(ErrorType::Runtime, message) {}
};

/**
 * @brief GC错误
 */
class GCError : public LuaError {
public:
    explicit GCError(const std::string& message = "Garbage collection error")
        : LuaError(ErrorType::Runtime, message) {}
};

/* ========================================================================== */
/* GC状态和配置 */
/* ========================================================================== */

/**
 * @brief GC状态枚举
 */
enum class GCState {
    Pause,          // 暂停状态
    Propagate,      // 传播标记状态  
    AtomicMark,     // 原子标记状态
    Sweep,          // 清除状态
    Finalize        // 终结状态
};

/**
 * @brief GC对象类型
 */
enum class GCObjectType {
    String,         // 字符串对象
    Table,          // 表对象
    Function,       // 函数对象
    UserData,       // 用户数据对象
    Thread,         // 协程对象
    Proto           // 函数原型对象
};

/**
 * @brief GC对象颜色（三色标记算法）
 */
enum class GCColor {
    White,          // 白色：未标记
    Gray,           // 灰色：已标记但子对象未扫描
    Black           // 黑色：已标记且子对象已扫描
};

/**
 * @brief 弱引用模式
 */
enum class WeakMode {
    None,           // 非弱引用
    Keys,           // 弱键
    Values,         // 弱值
    KeysAndValues   // 键值都弱
};

/**
 * @brief GC配置结构
 */
struct GCConfig {
    Size initial_threshold = 1024;         // 初始GC阈值（字节）
    int step_multiplier = 200;             // 步长乘数（百分比）
    int pause_multiplier = 200;            // 暂停乘数（百分比）
    bool enable_incremental = true;        // 启用增量GC
    bool enable_generational = false;      // 启用分代GC
    bool enable_auto_gc = true;           // 启用自动GC
    Size memory_limit = 0;                 // 内存限制（0为无限制）
    double target_pause_time = 0.01;       // 目标暂停时间（秒）
};

/* ========================================================================== */
/* GC对象基类 */
/* ========================================================================== */

/**
 * @brief GC对象基类
 * 
 * 所有需要垃圾回收的对象都继承自此类
 */
class GCObject {
public:
    /**
     * @brief 构造函数
     * @param type 对象类型
     * @param size 对象大小
     */
    explicit GCObject(GCObjectType type, Size size = 0);
    
    /**
     * @brief 虚析构函数
     */
    virtual ~GCObject() = default;
    
    // 禁用拷贝和移动
    GCObject(const GCObject&) = delete;
    GCObject& operator=(const GCObject&) = delete;
    GCObject(GCObject&&) = delete;
    GCObject& operator=(GCObject&&) = delete;
    
    /* ====================================================================== */
    /* 基本属性 */
    /* ====================================================================== */
    
    /**
     * @brief 获取对象类型
     */
    GCObjectType GetType() const { return type_; }
    
    /**
     * @brief 获取对象大小
     */
    Size GetSize() const { return size_; }
    
    /**
     * @brief 设置对象大小
     */
    void SetSize(Size size) { size_ = size; }
    
    /**
     * @brief 获取对象颜色
     */
    GCColor GetColor() const { return color_; }
    
    /**
     * @brief 设置对象颜色
     */
    void SetColor(GCColor color) { color_ = color; }
    
    /**
     * @brief 检查是否已标记
     */
    bool IsMarked() const { return color_ != GCColor::White; }
    
    /* ====================================================================== */
    /* 标记和遍历 */
    /* ====================================================================== */
    
    /**
     * @brief 标记对象（纯虚函数）
     * @param gc 垃圾收集器实例
     */
    virtual void Mark(class GarbageCollector* gc) = 0;
    
    /**
     * @brief 获取对象引用的所有子对象
     * @return 子对象列表
     */
    virtual std::vector<GCObject*> GetReferences() const = 0;
    
    /**
     * @brief 清理对象内部资源（在回收前调用）
     */
    virtual void Cleanup() {}
    
    /* ====================================================================== */
    /* 终结器支持 */
    /* ====================================================================== */
    
    /**
     * @brief 终结器函数类型
     */
    using Finalizer = std::function<void(GCObject*)>;
    
    /**
     * @brief 设置终结器
     */
    void SetFinalizer(const Finalizer& finalizer) { finalizer_ = finalizer; }
    
    /**
     * @brief 检查是否有终结器
     */
    bool HasFinalizer() const { return static_cast<bool>(finalizer_); }
    
    /**
     * @brief 调用终结器
     */
    void CallFinalizer();
    
    /* ====================================================================== */
    /* 弱引用支持 */
    /* ====================================================================== */
    
    /**
     * @brief 检查是否为弱引用对象
     */
    virtual bool IsWeak() const { return false; }
    
    /**
     * @brief 获取弱引用模式
     */
    virtual WeakMode GetWeakMode() const { return WeakMode::None; }
    
    /* ====================================================================== */
    /* 调试信息 */
    /* ====================================================================== */
    
    /**
     * @brief 获取对象的字符串表示
     */
    virtual std::string ToString() const;
    
    /**
     * @brief 获取对象的调试信息
     */
    virtual std::string GetDebugInfo() const;

private:
    GCObjectType type_;         // 对象类型
    Size size_;                 // 对象大小
    GCColor color_;            // 对象颜色
    Finalizer finalizer_;       // 终结器函数
    
    // GC链表指针（由GC管理）
    friend class GarbageCollector;
    GCObject* gc_next_;
    GCObject* gc_prev_;
};

/* ========================================================================== */
/* 具体GC对象类型 */
/* ========================================================================== */

/**
 * @brief 字符串对象
 */
class StringObject : public GCObject {
public:
    explicit StringObject(const std::string& str);
    
    const std::string& GetString() const { return str_; }
    void Mark(GarbageCollector* gc) override;
    std::vector<GCObject*> GetReferences() const override;
    std::string ToString() const override;

private:
    std::string str_;
};

/**
 * @brief 表对象
 */
class TableObject : public GCObject {
public:
    explicit TableObject(Size array_size = 0, Size hash_size = 0);
    
    void Set(const LuaValue& key, const LuaValue& value);
    LuaValue Get(const LuaValue& key) const;
    Size Size() const;
    Size GetArraySize() const { return array_size_; }
    Size GetHashSize() const { return hash_size_; }
    
    void Mark(GarbageCollector* gc) override;
    std::vector<GCObject*> GetReferences() const override;

private:
    Size array_size_;
    Size hash_size_;
    std::shared_ptr<LuaTable> table_;
};

/**
 * @brief 函数对象
 */
class FunctionObject : public GCObject {
public:
    explicit FunctionObject(const class Proto* proto);
    
    const class Proto* GetProto() const { return proto_; }
    void Mark(GarbageCollector* gc) override;
    std::vector<GCObject*> GetReferences() const override;

private:
    const class Proto* proto_;
    std::vector<LuaValue> upvalues_;
};

/**
 * @brief 用户数据对象
 */
class UserDataObject : public GCObject {
public:
    explicit UserDataObject(Size size);
    
    void* GetData() { return data_.get(); }
    const void* GetData() const { return data_.get(); }
    
    void Mark(GarbageCollector* gc) override;
    std::vector<GCObject*> GetReferences() const override;

private:
    std::unique_ptr<uint8_t[]> data_;
};

/**
 * @brief 弱引用表对象
 */
class WeakTableObject : public TableObject {
public:
    explicit WeakTableObject(WeakMode mode, Size array_size = 0, Size hash_size = 0);
    
    bool IsWeak() const override { return true; }
    WeakMode GetWeakMode() const override { return weak_mode_; }
    
    void Mark(GarbageCollector* gc) override;
    void CleanWeakReferences(GarbageCollector* gc);

private:
    WeakMode weak_mode_;
};

/* ========================================================================== */
/* GC统计信息 */
/* ========================================================================== */

/**
 * @brief GC统计信息
 */
struct GCStatistics {
    Size total_collections = 0;        // 总回收次数
    Size incremental_steps = 0;        // 增量步骤数
    Size total_allocated = 0;          // 总分配字节数
    Size total_freed = 0;              // 总释放字节数
    Size peak_memory_usage = 0;        // 峰值内存使用
    Size current_memory_usage = 0;     // 当前内存使用
    double total_gc_time = 0.0;        // 总GC时间（秒）
    double average_pause_time = 0.0;   // 平均暂停时间（秒）
    double max_pause_time = 0.0;       // 最大暂停时间（秒）
    double fragmentation_ratio = 0.0;  // 内存碎片率
    double memory_efficiency = 1.0;    // 内存效率
    Size objects_marked = 0;           // 标记的对象数
    Size objects_swept = 0;            // 清除的对象数
    Size finalizers_run = 0;           // 运行的终结器数
};

/* ========================================================================== */
/* 垃圾收集器主类 */
/* ========================================================================== */

/**
 * @brief 垃圾收集器类
 * 
 * 实现Lua 5.1.5的垃圾收集器，支持：
 * - 三色标记清除算法
 * - 增量垃圾回收
 * - 弱引用处理
 * - 终结器管理
 * - 多线程安全
 */
class GarbageCollector {
public:
    /**
     * @brief 构造函数
     * @param config GC配置
     */
    explicit GarbageCollector(const GCConfig& config = GCConfig());
    
    /**
     * @brief 析构函数
     */
    ~GarbageCollector();
    
    // 禁用拷贝，允许移动
    GarbageCollector(const GarbageCollector&) = delete;
    GarbageCollector& operator=(const GarbageCollector&) = delete;
    GarbageCollector(GarbageCollector&&) = default;
    GarbageCollector& operator=(GarbageCollector&&) = default;
    
    /* ====================================================================== */
    /* 对象分配 */
    /* ====================================================================== */
    
    /**
     * @brief 分配字符串对象
     */
    StringObject* AllocateString(const std::string& str);
    
    /**
     * @brief 分配表对象
     */
    TableObject* AllocateTable(Size array_size = 0, Size hash_size = 0);
    
    /**
     * @brief 分配函数对象
     */
    FunctionObject* AllocateFunction(const class Proto* proto);
    
    /**
     * @brief 分配用户数据对象
     */
    UserDataObject* AllocateUserData(Size size);
    
    /**
     * @brief 分配弱引用表对象
     */
    WeakTableObject* AllocateWeakTable(WeakMode mode, Size array_size = 0, Size hash_size = 0);
    
    /* ====================================================================== */
    /* GC控制 */
    /* ====================================================================== */
    
    /**
     * @brief 执行完整的垃圾回收
     * @param vm 主虚拟机
     * @param additional_vms 额外的虚拟机列表
     */
    void CollectGarbage(VirtualMachine* vm, 
                       const std::vector<VirtualMachine*>& additional_vms = {});
    
    /**
     * @brief 开始增量垃圾回收
     */
    void StartIncrementalCollection(VirtualMachine* vm);
    
    /**
     * @brief 执行增量GC步骤
     * @param vm 虚拟机
     * @param work_limit 工作量限制（字节）
     * @return 是否完成了完整的GC循环
     */
    bool IncrementalStep(VirtualMachine* vm, Size work_limit);
    
    /**
     * @brief 写屏障（用于增量GC）
     * @param parent 父对象
     * @param child 子对象
     */
    void WriteBarrier(GCObject* parent, GCObject* child);
    
    /* ====================================================================== */
    /* 状态管理 */
    /* ====================================================================== */
    
    /**
     * @brief 获取GC状态
     */
    GCState GetState() const { return state_; }
    
    /**
     * @brief 设置GC状态
     */
    void SetState(GCState state) { state_ = state; }
    
    /**
     * @brief 获取已分配字节数
     */
    Size GetAllocatedBytes() const { return allocated_bytes_.load(); }
    
    /**
     * @brief 获取对象总数
     */
    Size GetTotalObjects() const { return total_objects_.load(); }
    
    /**
     * @brief 获取回收次数
     */
    Size GetCollectionCount() const { return collection_count_.load(); }
    
    /* ====================================================================== */
    /* 配置管理 */
    /* ====================================================================== */
    
    /**
     * @brief 获取GC阈值
     */
    Size GetThreshold() const { return threshold_; }
    
    /**
     * @brief 设置GC阈值
     */
    void SetThreshold(Size threshold) { threshold_ = threshold; }
    
    /**
     * @brief 获取步长乘数
     */
    int GetStepMultiplier() const { return config_.step_multiplier; }
    
    /**
     * @brief 设置步长乘数
     */
    void SetStepMultiplier(int multiplier) { config_.step_multiplier = multiplier; }
    
    /**
     * @brief 获取暂停乘数
     */
    int GetPause() const { return config_.pause_multiplier; }
    
    /**
     * @brief 设置暂停乘数
     */
    void SetPause(int pause) { config_.pause_multiplier = pause; }
    
    /**
     * @brief 检查是否启用增量GC
     */
    bool IsIncrementalEnabled() const { return config_.enable_incremental; }
    
    /**
     * @brief 检查是否启用分代GC
     */
    bool IsGenerationalEnabled() const { return config_.enable_generational; }
    
    /**
     * @brief 检查是否启用自动GC
     */
    bool IsAutomaticGCEnabled() const { return config_.enable_auto_gc; }
    
    /**
     * @brief 设置自动GC
     */
    void SetAutomaticGC(bool enabled) { config_.enable_auto_gc = enabled; }
    
    /**
     * @brief 获取内存限制
     */
    Size GetMemoryLimit() const { return config_.memory_limit; }
    
    /**
     * @brief 设置内存限制
     */
    void SetMemoryLimit(Size limit) { config_.memory_limit = limit; }
    
    /* ====================================================================== */
    /* 统计和调试 */
    /* ====================================================================== */
    
    /**
     * @brief 获取GC统计信息
     */
    const GCStatistics& GetStatistics() const { return statistics_; }
    
    /**
     * @brief 重置统计信息
     */
    void ResetStatistics();
    
    /**
     * @brief 设置对象复活回调
     */
    using ResurrectionCallback = std::function<bool(GCObject*)>;
    void SetResurrectionCallback(const ResurrectionCallback& callback) {
        resurrection_callback_ = callback;
    }
    
    /**
     * @brief 获取内存使用报告
     */
    std::string GetMemoryReport() const;
    
    /**
     * @brief 验证堆完整性
     */
    bool ValidateHeap() const;

private:
    /* ====================================================================== */
    /* 内部GC算法 */
    /* ====================================================================== */
    
    /**
     * @brief 标记阶段
     */
    void MarkPhase(VirtualMachine* vm, const std::vector<VirtualMachine*>& additional_vms);
    
    /**
     * @brief 原子标记阶段
     */
    void AtomicMarkPhase();
    
    /**
     * @brief 清除阶段
     */
    void SweepPhase();
    
    /**
     * @brief 终结阶段
     */
    void FinalizePhase();
    
    /**
     * @brief 标记根对象
     */
    void MarkRoots(VirtualMachine* vm, const std::vector<VirtualMachine*>& additional_vms);
    
    /**
     * @brief 传播标记
     */
    Size PropagateMarks(Size work_limit = SIZE_MAX);
    
    /**
     * @brief 清理弱引用
     */
    void CleanupWeakReferences();
    
    /**
     * @brief 运行终结器
     */
    void RunFinalizers();
    
    /* ====================================================================== */
    /* 内存管理 */
    /* ====================================================================== */
    
    /**
     * @brief 分配内存
     */
    void* AllocateMemory(Size size);
    
    /**
     * @brief 释放内存
     */
    void FreeMemory(void* ptr, Size size);
    
    /**
     * @brief 注册对象
     */
    void RegisterObject(GCObject* obj);
    
    /**
     * @brief 注销对象
     */
    void UnregisterObject(GCObject* obj);
    
    /**
     * @brief 检查是否应该触发GC
     */
    bool ShouldTriggerGC() const;
    
    /**
     * @brief 更新阈值
     */
    void UpdateThreshold();
    
    /* ====================================================================== */
    /* 并发控制 */
    /* ====================================================================== */
    
    /**
     * @brief 获取分配锁
     */
    void LockAllocation();
    
    /**
     * @brief 释放分配锁
     */
    void UnlockAllocation();
    
    /* ====================================================================== */
    /* 成员变量 */
    /* ====================================================================== */
    
    // 配置和状态
    GCConfig config_;                           // GC配置
    GCState state_;                             // 当前GC状态
    Size threshold_;                            // GC触发阈值
    
    // 对象管理
    GCObject* all_objects_;                     // 所有对象链表
    std::unordered_set<GCObject*> gray_objects_; // 灰色对象集合
    std::vector<GCObject*> to_finalize_;        // 待终结对象
    std::vector<WeakTableObject*> weak_tables_; // 弱引用表
    
    // 统计信息（原子操作确保线程安全）
    std::atomic<Size> allocated_bytes_;         // 已分配字节数
    std::atomic<Size> total_objects_;           // 对象总数
    std::atomic<Size> collection_count_;        // 回收次数
    GCStatistics statistics_;                   // 详细统计
    
    // 增量GC状态
    std::vector<GCObject*>::iterator sweep_iterator_; // 清除迭代器
    Size incremental_debt_;                     // 增量债务
    
    // 回调函数
    ResurrectionCallback resurrection_callback_; // 对象复活回调
    
    // 并发控制
    mutable std::mutex allocation_mutex_;       // 分配互斥锁
    mutable std::mutex gc_mutex_;               // GC互斥锁
    
    // 性能监控
    std::chrono::high_resolution_clock::time_point last_gc_time_;
};

/* ========================================================================== */
/* 工厂函数 */
/* ========================================================================== */

/**
 * @brief 创建标准垃圾收集器
 */
std::unique_ptr<GarbageCollector> CreateStandardGC();

/**
 * @brief 创建高性能垃圾收集器
 */
std::unique_ptr<GarbageCollector> CreateHighPerformanceGC();

/**
 * @brief 创建低延迟垃圾收集器
 */
std::unique_ptr<GarbageCollector> CreateLowLatencyGC();

/**
 * @brief 创建嵌入式垃圾收集器
 */
std::unique_ptr<GarbageCollector> CreateEmbeddedGC();

} // namespace lua_cpp