/**
 * @file memory_manager.h
 * @brief 内存管理器接口
 * @description 提供统一的内存分配和管理接口
 * @author Lua C++ Project
 * @date 2025-09-21
 */

#pragma once

#include "core/lua_common.h"
#include "garbage_collector.h"
#include <memory>
#include <functional>
#include <atomic>
#include <mutex>

namespace lua_cpp {

/* ========================================================================== */
/* 内存分配策略 */
/* ========================================================================== */

/**
 * @brief 内存分配策略枚举
 */
enum class AllocationStrategy {
    System,         // 使用系统默认分配器
    Pool,           // 使用内存池
    Stack,          // 使用栈分配器
    Custom          // 使用自定义分配器
};

/**
 * @brief 内存对齐策略
 */
enum class AlignmentStrategy {
    Natural,        // 自然对齐
    Cache,          // 缓存行对齐
    Page,           // 页对齐
    Custom          // 自定义对齐
};

/* ========================================================================== */
/* 内存统计信息 */
/* ========================================================================== */

/**
 * @brief 内存使用统计
 */
struct MemoryStats {
    Size total_allocated = 0;          // 总分配字节数
    Size total_freed = 0;              // 总释放字节数
    Size current_usage = 0;            // 当前使用量
    Size peak_usage = 0;               // 峰值使用量
    Size allocation_count = 0;         // 分配次数
    Size deallocation_count = 0;       // 释放次数
    double fragmentation_ratio = 0.0;  // 碎片率
    Size largest_free_block = 0;       // 最大自由块
    Size smallest_free_block = 0;      // 最小自由块
    Size free_block_count = 0;         // 自由块数量
};

/* ========================================================================== */
/* 内存分配器接口 */
/* ========================================================================== */

/**
 * @brief 抽象内存分配器接口
 */
class Allocator {
public:
    virtual ~Allocator() = default;
    
    /**
     * @brief 分配内存
     * @param size 请求的字节数
     * @param alignment 对齐要求
     * @return 分配的内存指针，失败时返回nullptr
     */
    virtual void* Allocate(Size size, Size alignment = sizeof(void*)) = 0;
    
    /**
     * @brief 释放内存
     * @param ptr 要释放的内存指针
     * @param size 内存大小（某些分配器需要）
     */
    virtual void Deallocate(void* ptr, Size size = 0) = 0;
    
    /**
     * @brief 重新分配内存
     * @param ptr 原内存指针
     * @param old_size 原大小
     * @param new_size 新大小
     * @param alignment 对齐要求
     * @return 新的内存指针
     */
    virtual void* Reallocate(void* ptr, Size old_size, Size new_size, Size alignment = sizeof(void*)) = 0;
    
    /**
     * @brief 获取分配器名称
     */
    virtual const char* GetName() const = 0;
    
    /**
     * @brief 获取内存统计
     */
    virtual MemoryStats GetStats() const = 0;
    
    /**
     * @brief 重置统计信息
     */
    virtual void ResetStats() = 0;
};

/* ========================================================================== */
/* 系统分配器 */
/* ========================================================================== */

/**
 * @brief 系统默认分配器
 */
class SystemAllocator : public Allocator {
public:
    SystemAllocator();
    ~SystemAllocator() override;
    
    void* Allocate(Size size, Size alignment = sizeof(void*)) override;
    void Deallocate(void* ptr, Size size = 0) override;
    void* Reallocate(void* ptr, Size old_size, Size new_size, Size alignment = sizeof(void*)) override;
    
    const char* GetName() const override { return "SystemAllocator"; }
    MemoryStats GetStats() const override;
    void ResetStats() override;

private:
    mutable std::mutex stats_mutex_;
    MemoryStats stats_;
};

/* ========================================================================== */
/* 内存池分配器 */
/* ========================================================================== */

/**
 * @brief 固定大小内存池
 */
class FixedPoolAllocator : public Allocator {
public:
    /**
     * @brief 构造函数
     * @param block_size 块大小
     * @param block_count 块数量
     */
    FixedPoolAllocator(Size block_size, Size block_count);
    ~FixedPoolAllocator() override;
    
    void* Allocate(Size size, Size alignment = sizeof(void*)) override;
    void Deallocate(void* ptr, Size size = 0) override;
    void* Reallocate(void* ptr, Size old_size, Size new_size, Size alignment = sizeof(void*)) override;
    
    const char* GetName() const override { return "FixedPoolAllocator"; }
    MemoryStats GetStats() const override;
    void ResetStats() override;
    
    /**
     * @brief 检查指针是否属于此池
     */
    bool OwnsPointer(void* ptr) const;

private:
    void InitializePool();
    void CleanupPool();
    
    Size block_size_;
    Size block_count_;
    void* pool_memory_;
    void* free_list_;
    mutable std::mutex pool_mutex_;
    MemoryStats stats_;
};

/* ========================================================================== */
/* 栈分配器 */
/* ========================================================================== */

/**
 * @brief 栈式内存分配器
 */
class StackAllocator : public Allocator {
public:
    /**
     * @brief 构造函数
     * @param capacity 栈容量
     */
    explicit StackAllocator(Size capacity);
    ~StackAllocator() override;
    
    void* Allocate(Size size, Size alignment = sizeof(void*)) override;
    void Deallocate(void* ptr, Size size = 0) override;
    void* Reallocate(void* ptr, Size old_size, Size new_size, Size alignment = sizeof(void*)) override;
    
    const char* GetName() const override { return "StackAllocator"; }
    MemoryStats GetStats() const override;
    void ResetStats() override;
    
    /**
     * @brief 保存栈状态
     */
    struct Marker {
        Size position;
    };
    
    /**
     * @brief 获取当前栈标记
     */
    Marker GetMarker() const;
    
    /**
     * @brief 回滚到指定标记
     */
    void RollbackToMarker(const Marker& marker);
    
    /**
     * @brief 清空栈
     */
    void Clear();

private:
    void* stack_memory_;
    Size capacity_;
    Size current_position_;
    mutable std::mutex stack_mutex_;
    MemoryStats stats_;
};

/* ========================================================================== */
/* 内存管理器主类 */
/* ========================================================================== */

/**
 * @brief 内存管理器
 * 
 * 统一管理内存分配、垃圾收集和内存监控
 */
class MemoryManager {
public:
    /**
     * @brief 构造函数
     */
    MemoryManager();
    
    /**
     * @brief 析构函数
     */
    ~MemoryManager();
    
    // 禁用拷贝和移动
    LUA_NO_COPY_MOVE(MemoryManager)
    
    /* ====================================================================== */
    /* 分配器管理 */
    /* ====================================================================== */
    
    /**
     * @brief 设置默认分配器
     */
    void SetDefaultAllocator(std::unique_ptr<Allocator> allocator);
    
    /**
     * @brief 获取默认分配器
     */
    Allocator* GetDefaultAllocator() const { return default_allocator_.get(); }
    
    /**
     * @brief 注册命名分配器
     */
    void RegisterAllocator(const std::string& name, std::unique_ptr<Allocator> allocator);
    
    /**
     * @brief 获取命名分配器
     */
    Allocator* GetAllocator(const std::string& name) const;
    
    /* ====================================================================== */
    /* 内存分配接口 */
    /* ====================================================================== */
    
    /**
     * @brief 分配内存
     */
    void* Allocate(Size size, Size alignment = sizeof(void*), const std::string& allocator_name = "");
    
    /**
     * @brief 释放内存
     */
    void Deallocate(void* ptr, Size size = 0, const std::string& allocator_name = "");
    
    /**
     * @brief 重新分配内存
     */
    void* Reallocate(void* ptr, Size old_size, Size new_size, Size alignment = sizeof(void*), const std::string& allocator_name = "");
    
    /* ====================================================================== */
    /* GC对象分配 */
    /* ====================================================================== */
    
    /**
     * @brief 分配GC对象内存
     */
    template<typename T, typename... Args>
    T* AllocateGCObject(Args&&... args) {
        static_assert(std::is_base_of_v<GCObject, T>, "T must inherit from GCObject");
        
        void* memory = Allocate(sizeof(T), alignof(T));
        if (!memory) {
            throw OutOfMemoryError("Failed to allocate memory for GC object");
        }
        
        T* obj = new(memory) T(std::forward<Args>(args)...);
        
        if (garbage_collector_) {
            garbage_collector_->RegisterObject(obj);
        }
        
        return obj;
    }
    
    /**
     * @brief 释放GC对象内存
     */
    template<typename T>
    void DeallocateGCObject(T* obj) {
        static_assert(std::is_base_of_v<GCObject, T>, "T must inherit from GCObject");
        
        if (!obj) return;
        
        if (garbage_collector_) {
            garbage_collector_->UnregisterObject(obj);
        }
        
        obj->~T();
        Deallocate(obj, sizeof(T));
    }
    
    /* ====================================================================== */
    /* 垃圾收集器集成 */
    /* ====================================================================== */
    
    /**
     * @brief 设置垃圾收集器
     */
    void SetGarbageCollector(std::unique_ptr<GarbageCollector> gc);
    
    /**
     * @brief 获取垃圾收集器
     */
    GarbageCollector* GetGarbageCollector() const { return garbage_collector_.get(); }
    
    /**
     * @brief 触发垃圾收集
     */
    void CollectGarbage();
    
    /* ====================================================================== */
    /* 内存监控 */
    /* ====================================================================== */
    
    /**
     * @brief 获取总内存统计
     */
    MemoryStats GetTotalStats() const;
    
    /**
     * @brief 获取分配器统计
     */
    std::map<std::string, MemoryStats> GetAllocatorStats() const;
    
    /**
     * @brief 设置内存限制
     */
    void SetMemoryLimit(Size limit) { memory_limit_ = limit; }
    
    /**
     * @brief 获取内存限制
     */
    Size GetMemoryLimit() const { return memory_limit_; }
    
    /**
     * @brief 检查是否超出内存限制
     */
    bool IsMemoryLimitExceeded() const;
    
    /* ====================================================================== */
    /* 内存报告和调试 */
    /* ====================================================================== */
    
    /**
     * @brief 生成内存使用报告
     */
    std::string GenerateMemoryReport() const;
    
    /**
     * @brief 转储内存统计
     */
    void DumpMemoryStats() const;
    
    /**
     * @brief 验证内存完整性
     */
    bool ValidateMemoryIntegrity() const;
    
    /* ====================================================================== */
    /* 内存事件回调 */
    /* ====================================================================== */
    
    using AllocationCallback = std::function<void(void*, Size)>;
    using DeallocationCallback = std::function<void(void*, Size)>;
    using OutOfMemoryCallback = std::function<void(Size)>;
    
    /**
     * @brief 设置分配回调
     */
    void SetAllocationCallback(const AllocationCallback& callback) {
        allocation_callback_ = callback;
    }
    
    /**
     * @brief 设置释放回调
     */
    void SetDeallocationCallback(const DeallocationCallback& callback) {
        deallocation_callback_ = callback;
    }
    
    /**
     * @brief 设置内存不足回调
     */
    void SetOutOfMemoryCallback(const OutOfMemoryCallback& callback) {
        out_of_memory_callback_ = callback;
    }

private:
    /* ====================================================================== */
    /* 内部方法 */
    /* ====================================================================== */
    
    /**
     * @brief 初始化默认分配器
     */
    void InitializeDefaultAllocators();
    
    /**
     * @brief 更新统计信息
     */
    void UpdateStats(Size allocated, Size deallocated);
    
    /**
     * @brief 检查内存限制
     */
    void CheckMemoryLimit(Size requested_size);
    
    /* ====================================================================== */
    /* 成员变量 */
    /* ====================================================================== */
    
    // 分配器管理
    std::unique_ptr<Allocator> default_allocator_;
    std::map<std::string, std::unique_ptr<Allocator>> named_allocators_;
    
    // 垃圾收集器
    std::unique_ptr<GarbageCollector> garbage_collector_;
    
    // 内存限制和统计
    std::atomic<Size> memory_limit_;
    std::atomic<Size> total_allocated_;
    std::atomic<Size> total_deallocated_;
    
    // 事件回调
    AllocationCallback allocation_callback_;
    DeallocationCallback deallocation_callback_;
    OutOfMemoryCallback out_of_memory_callback_;
    
    // 线程安全
    mutable std::mutex manager_mutex_;
};

/* ========================================================================== */
/* 全局内存管理器访问 */
/* ========================================================================== */

/**
 * @brief 获取全局内存管理器实例
 */
MemoryManager& GetGlobalMemoryManager();

/**
 * @brief 设置全局内存管理器
 */
void SetGlobalMemoryManager(std::unique_ptr<MemoryManager> manager);

/* ========================================================================== */
/* 便捷分配函数 */
/* ========================================================================== */

/**
 * @brief 分配内存的便捷函数
 */
template<typename T>
T* AllocateMemory(Size count = 1) {
    return static_cast<T*>(GetGlobalMemoryManager().Allocate(sizeof(T) * count, alignof(T)));
}

/**
 * @brief 释放内存的便捷函数
 */
template<typename T>
void DeallocateMemory(T* ptr, Size count = 1) {
    GetGlobalMemoryManager().Deallocate(ptr, sizeof(T) * count);
}

/**
 * @brief 分配GC对象的便捷函数
 */
template<typename T, typename... Args>
T* AllocateGCObject(Args&&... args) {
    return GetGlobalMemoryManager().AllocateGCObject<T>(std::forward<Args>(args)...);
}

/**
 * @brief 释放GC对象的便捷函数
 */
template<typename T>
void DeallocateGCObject(T* obj) {
    GetGlobalMemoryManager().DeallocateGCObject(obj);
}

} // namespace lua_cpp