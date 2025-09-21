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
#include <mutex>

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
        : LuaError(ErrorCode::OutOfMemory, message) {}
};

/**
 * @brief GC错误
 */
class GCError : public LuaError {
public:
    explicit GCError(const std::string& message = "Garbage collection error")
        : LuaError(ErrorCode::Generic, message) {}
};

/* ========================================================================== */
/* GC状态和配置 */
/* ========================================================================== */

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
    bool IsMarked() const { return color_ != GCColor::White0 && color_ != GCColor::White1; }
    
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
    Size GetSize() const;
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

/* ========================================================================== */
/* GC统计信息结构 */
/* ========================================================================== */

/**
 * @brief GC统计信息
 */
struct GCStats {
    Size collections_performed = 0;    // 执行的回收次数
    Size total_freed_bytes = 0;        // 总释放字节数
    Size total_freed_objects = 0;      // 总释放对象数
    Size max_memory_used = 0;          // 最大内存使用量
    double average_pause_time = 0.0;   // 平均暂停时间
    Size current_memory_usage = 0;     // 当前内存使用量
    Size current_object_count = 0;     // 当前对象数量
    Size gc_threshold = 0;             // GC阈值
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
 * 实现标记-清扫垃圾收集器，支持：
 * - 三色标记算法
 * - 增量垃圾回收
 * - 终结器管理
 * - 线程安全
 */
class GarbageCollector {
public:
    /**
     * @brief 构造函数
     * @param vm 关联的虚拟机
     */
    explicit GarbageCollector(VirtualMachine* vm = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~GarbageCollector();
    
    // 禁用拷贝和移动
    LUA_NO_COPY_MOVE(GarbageCollector)
    
    /* ====================================================================== */
    /* 对象生命周期管理 */
    /* ====================================================================== */
    
    /**
     * @brief 注册GC对象
     */
    void RegisterObject(GCObject* obj);
    
    /**
     * @brief 注销GC对象
     */
    void UnregisterObject(GCObject* obj);
    
    /* ====================================================================== */
    /* 垃圾收集控制 */
    /* ====================================================================== */
    
    /**
     * @brief 执行垃圾收集
     */
    void Collect();
    
    /**
     * @brief 执行完整收集
     */
    void PerformFullCollection();
    
    /**
     * @brief 执行增量收集
     */
    void PerformIncrementalCollection();
    
    /**
     * @brief 触发垃圾收集
     */
    void TriggerGC();
    
    /* ====================================================================== */
    /* 标记阶段方法 */
    /* ====================================================================== */
    
    /**
     * @brief 标记阶段
     */
    void MarkPhase();
    
    /**
     * @brief 重置对象颜色
     */
    void ResetColors();
    
    /**
     * @brief 标记根对象
     */
    void MarkRoots();
    
    /**
     * @brief 标记VM栈
     */
    void MarkVMStack();
    
    /**
     * @brief 标记全局变量
     */
    void MarkGlobals();
    
    /**
     * @brief 标记调用栈
     */
    void MarkCallStack();
    
    /**
     * @brief 标记注册表
     */
    void MarkRegistry();
    
    /**
     * @brief 标记对象
     */
    void MarkObject(GCObject* obj);
    
    /**
     * @brief 传播标记
     */
    void PropagateMarks();
    
    /**
     * @brief 从对象传播标记
     */
    void PropagateMarkFrom(GCObject* obj);
    
    /* ====================================================================== */
    /* 清除阶段方法 */
    /* ====================================================================== */
    
    /**
     * @brief 清除阶段
     */
    void SweepPhase();
    
    /**
     * @brief 终结阶段
     */
    void FinalizePhase();
    
    /* ====================================================================== */
    /* 灰色列表管理 */
    /* ====================================================================== */
    
    /**
     * @brief 添加到灰色列表
     */
    void AddToGrayList(GCObject* obj);
    
    /**
     * @brief 从灰色列表弹出
     */
    GCObject* PopFromGrayList();
    
    /**
     * @brief 从灰色列表移除
     */
    void RemoveFromGrayList(GCObject* obj);
    
    /* ====================================================================== */
    /* 增量GC步骤 */
    /* ====================================================================== */
    
    /**
     * @brief 开始标记阶段
     */
    void StartMarkPhase();
    
    /**
     * @brief 执行标记步骤
     */
    bool PerformMarkStep();
    
    /**
     * @brief 执行原子标记
     */
    void PerformAtomicMark();
    
    /**
     * @brief 执行清除步骤
     */
    bool PerformSweepStep();
    
    /**
     * @brief 执行终结
     */
    void PerformFinalize();
    
    /* ====================================================================== */
    /* 触发条件和阈值 */
    /* ====================================================================== */
    
    /**
     * @brief 检查是否应该触发GC
     */
    bool ShouldTriggerGC() const;
    
    /**
     * @brief 检查是否应该开始收集
     */
    bool ShouldStartCollection() const;
    
    /**
     * @brief 调整GC阈值
     */
    void AdjustThreshold();
    
    /* ====================================================================== */
    /* 配置和状态 */
    /* ====================================================================== */
    
    /**
     * @brief 设置配置
     */
    void SetConfig(const GCConfig& config);
    
    /**
     * @brief 获取配置
     */
    GCConfig GetConfig() const;
    
    /**
     * @brief 获取统计信息
     */
    GCStats GetStats() const;
    
    /**
     * @brief 获取当前状态
     */
    GCState GetState() const { return state_; }
    
    /**
     * @brief 获取总字节数
     */
    Size GetTotalBytes() const { return total_bytes_; }
    
    /**
     * @brief 获取对象数量
     */
    Size GetObjectCount() const { return object_count_; }
    
    /* ====================================================================== */
    /* 调试和诊断 */
    /* ====================================================================== */
    
    /**
     * @brief 转储统计信息
     */
    void DumpStats() const;
    
    /**
     * @brief 转储对象信息
     */
    void DumpObjects() const;
    
    /**
     * @brief 检查一致性
     */
    bool CheckConsistency() const;
    
    /**
     * @brief 释放所有对象
     */
    void FreeAllObjects();

private:
    /* ====================================================================== */
    /* 成员变量 */
    /* ====================================================================== */
    
    VirtualMachine* vm_;                        // 关联的虚拟机
    GCConfig config_;                           // GC配置
    GCState state_;                             // 当前状态
    
    // 内存统计
    Size total_bytes_;                          // 总分配字节数
    Size gc_threshold_;                         // GC触发阈值
    
    // 时间统计
    std::chrono::steady_clock::time_point last_collection_time_;
    std::chrono::steady_clock::time_point pause_start_time_;
    Size collection_count_;
    
    // 对象管理
    Size object_count_;                         // 对象总数
    GCObject* all_objects_;                     // 所有对象链表头
    GCObject* gray_list_;                       // 灰色对象列表头
    Size gray_count_;                           // 灰色对象数量
    
    // 清除状态
    GCObject* sweep_current_;                   // 当前清除位置
    
    // 终结列表
    std::vector<GCObject*> finalization_list_; // 待终结对象列表
    
    // 统计信息
    GCStats stats_;
    
    // 线程安全
    mutable std::mutex gc_mutex_;
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