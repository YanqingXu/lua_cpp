#pragma once

#include "core/lua_common.h"
#include "types/value.h"
#include "core/lua_errors.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace lua_cpp {

/* ========================================================================== */
/* 前向声明 */
/* ========================================================================== */

class LuaStack;
class GarbageCollector;

/* ========================================================================== */
/* Upvalue错误类型 */
/* ========================================================================== */

/**
 * @brief Upvalue相关错误
 */
class UpvalueError : public LuaError {
public:
    explicit UpvalueError(const std::string& message = "Upvalue error")
        : LuaError(message, ErrorType::RUNTIME_ERROR) {}
};

/**
 * @brief Upvalue访问错误
 */
class UpvalueAccessError : public UpvalueError {
public:
    explicit UpvalueAccessError(const std::string& message = "Invalid upvalue access")
        : UpvalueError(message) {}
};

/* ========================================================================== */
/* Upvalue核心类 */
/* ========================================================================== */

/**
 * @brief Upvalue实体
 * 
 * Lua闭包中的上值，可以是开放的（指向栈上的值）或闭合的（持有副本）
 */
class Upvalue {
public:
    /**
     * @brief 构造函数（开放状态）
     * @param stack_index 栈索引
     * @param stack_ptr 栈指针（用于快速访问）
     */
    Upvalue(Size stack_index, LuaValue* stack_ptr);
    
    /**
     * @brief 构造函数（闭合状态）
     * @param closed_value 闭合的值
     */
    explicit Upvalue(const LuaValue& closed_value);
    
    /**
     * @brief 析构函数
     */
    ~Upvalue() = default;
    
    // 禁用拷贝，允许移动
    Upvalue(const Upvalue&) = delete;
    Upvalue& operator=(const Upvalue&) = delete;
    Upvalue(Upvalue&&) = default;
    Upvalue& operator=(Upvalue&&) = default;
    
    /* ====================================================================== */
    /* 值访问 */
    /* ====================================================================== */
    
    /**
     * @brief 获取值
     * @return 当前值的引用
     */
    LuaValue& GetValue();
    const LuaValue& GetValue() const;
    
    /**
     * @brief 设置值
     * @param value 新值
     */
    void SetValue(const LuaValue& value);
    void SetValue(LuaValue&& value);
    
    /**
     * @brief 获取值指针（用于快速访问）
     * @return 值指针
     */
    LuaValue* GetValuePtr();
    const LuaValue* GetValuePtr() const;
    
    /* ====================================================================== */
    /* 状态管理 */
    /* ====================================================================== */
    
    /**
     * @brief 检查是否已闭合
     */
    bool IsClosed() const { return is_closed_; }
    
    /**
     * @brief 获取栈索引（仅开放状态有效）
     */
    Size GetStackIndex() const { return stack_index_; }
    
    /**
     * @brief 闭合Upvalue
     * 将栈上的值复制到内部存储，断开与栈的连接
     */
    void Close();
    
    /**
     * @brief 检查是否指向特定栈位置
     * @param stack_index 栈索引
     * @return 是否匹配
     */
    bool PointsToStackIndex(Size stack_index) const;
    
    /* ====================================================================== */
    /* 引用计数 */
    /* ====================================================================== */
    
    /**
     * @brief 增加引用计数
     */
    void AddReference() { ref_count_++; }
    
    /**
     * @brief 减少引用计数
     * @return 新的引用计数
     */
    Size RemoveReference() { 
        if (ref_count_ > 0) ref_count_--; 
        return ref_count_; 
    }
    
    /**
     * @brief 获取引用计数
     */
    Size GetReferenceCount() const { return ref_count_; }
    
    /**
     * @brief 检查是否还有引用
     */
    bool HasReferences() const { return ref_count_ > 0; }
    
    /* ====================================================================== */
    /* 链表管理（用于UpvalueManager）*/
    /* ====================================================================== */
    
    /**
     * @brief 设置链表指针
     */
    void SetNext(Upvalue* next) { next_ = next; }
    void SetPrev(Upvalue* prev) { prev_ = prev; }
    
    /**
     * @brief 获取链表指针
     */
    Upvalue* GetNext() const { return next_; }
    Upvalue* GetPrev() const { return prev_; }
    
    /* ====================================================================== */
    /* 调试和诊断 */
    /* ====================================================================== */
    
    /**
     * @brief 获取调试信息
     */
    struct UpvalueInfo {
        bool is_closed;
        Size stack_index;
        Size ref_count;
        std::string value_type;
        std::string value_string;
        Size memory_usage;
    };
    
    /**
     * @brief 获取Upvalue信息
     */
    UpvalueInfo GetInfo() const;
    
    /**
     * @brief 获取字符串表示
     */
    std::string ToString() const;
    
    /**
     * @brief 验证Upvalue完整性
     */
    bool ValidateIntegrity() const;

private:
    /* ====================================================================== */
    /* 成员变量 */
    /* ====================================================================== */
    
    bool is_closed_;                    // 是否已闭合
    Size stack_index_;                  // 栈索引（开放状态）
    LuaValue* stack_value_ptr_;         // 栈值指针（开放状态）
    LuaValue closed_value_;             // 闭合值（闭合状态）
    Size ref_count_;                    // 引用计数
    
    // 链表指针（用于UpvalueManager的链式管理）
    Upvalue* next_;                     // 下一个Upvalue
    Upvalue* prev_;                     // 上一个Upvalue
};

/* ========================================================================== */
/* Upvalue管理器 */
/* ========================================================================== */

/**
 * @brief Upvalue管理器
 * 
 * 管理所有Upvalue的生命周期，实现Lua闭包的正确语义：
 * - 开放Upvalue共享机制
 * - 自动闭合管理
 * - 内存回收和垃圾收集
 * - 性能优化
 */
class UpvalueManager {
public:
    /**
     * @brief 构造函数
     * @param stack 关联的Lua栈
     */
    explicit UpvalueManager(LuaStack* stack);
    
    /**
     * @brief 析构函数
     */
    ~UpvalueManager();
    
    // 禁用拷贝，允许移动
    UpvalueManager(const UpvalueManager&) = delete;
    UpvalueManager& operator=(const UpvalueManager&) = delete;
    UpvalueManager(UpvalueManager&&) = default;
    UpvalueManager& operator=(UpvalueManager&&) = default;
    
    /* ====================================================================== */
    /* 核心Upvalue操作 */
    /* ====================================================================== */
    
    /**
     * @brief 获取或创建Upvalue
     * @param stack_index 栈索引
     * @return Upvalue指针
     */
    std::shared_ptr<Upvalue> GetUpvalue(Size stack_index);
    
    /**
     * @brief 创建新的Upvalue
     * @param stack_index 栈索引
     * @return Upvalue指针
     */
    std::shared_ptr<Upvalue> CreateUpvalue(Size stack_index);
    
    /**
     * @brief 查找现有Upvalue
     * @param stack_index 栈索引
     * @return Upvalue指针，如果不存在返回nullptr
     */
    std::shared_ptr<Upvalue> FindUpvalue(Size stack_index);
    
    /**
     * @brief 闭合指定级别以上的Upvalue
     * @param level 栈级别
     */
    void CloseUpvalues(Size level);
    
    /**
     * @brief 闭合所有开放的Upvalue
     */
    void CloseAllUpvalues();
    
    /**
     * @brief 移除Upvalue
     * @param upvalue Upvalue指针
     */
    void RemoveUpvalue(std::shared_ptr<Upvalue> upvalue);
    
    /* ====================================================================== */
    /* 生命周期管理 */
    /* ====================================================================== */
    
    /**
     * @brief 添加Upvalue引用
     * @param upvalue Upvalue指针
     */
    void AddReference(std::shared_ptr<Upvalue> upvalue);
    
    /**
     * @brief 移除Upvalue引用
     * @param upvalue Upvalue指针
     * @return 是否已被回收
     */
    bool RemoveReference(std::shared_ptr<Upvalue> upvalue);
    
    /**
     * @brief 清理无引用的Upvalue
     * @return 清理的数量
     */
    Size CleanupUnreferencedUpvalues();
    
    /**
     * @brief 强制垃圾收集
     * @return 回收的Upvalue数量
     */
    Size ForceGarbageCollection();
    
    /* ====================================================================== */
    /* 批量操作 */
    /* ====================================================================== */
    
    /**
     * @brief 更新栈引用（当栈重新分配时）
     * @param old_stack 旧栈指针
     * @param new_stack 新栈指针
     */
    void UpdateStackReferences(LuaValue* old_stack, LuaValue* new_stack);
    
    /**
     * @brief 迁移Upvalue（当栈位置改变时）
     * @param old_index 旧索引
     * @param new_index 新索引
     */
    void MigrateUpvalue(Size old_index, Size new_index);
    
    /**
     * @brief 清空所有Upvalue
     */
    void Clear();
    
    /* ====================================================================== */
    /* 查询和统计 */
    /* ====================================================================== */
    
    /**
     * @brief 获取Upvalue数量
     */
    Size GetUpvalueCount() const { return upvalue_map_.size(); }
    
    /**
     * @brief 获取开放Upvalue数量
     */
    Size GetOpenUpvalueCount() const;
    
    /**
     * @brief 获取闭合Upvalue数量
     */
    Size GetClosedUpvalueCount() const;
    
    /**
     * @brief 获取总引用计数
     */
    Size GetTotalReferenceCount() const;
    
    /**
     * @brief 获取内存使用量
     * @return 字节数
     */
    Size GetMemoryUsage() const;
    
    /**
     * @brief 检查是否为空
     */
    bool IsEmpty() const { return upvalue_map_.empty(); }
    
    /* ====================================================================== */
    /* 性能统计 */
    /* ====================================================================== */
    
    /**
     * @brief Upvalue统计信息
     */
    struct UpvalueStatistics {
        Size total_upvalues = 0;            // 总Upvalue数
        Size open_upvalues = 0;             // 开放Upvalue数
        Size closed_upvalues = 0;           // 闭合Upvalue数
        Size total_references = 0;          // 总引用数
        Size unreferenced_upvalues = 0;     // 无引用Upvalue数
        
        Size upvalues_created = 0;          // 创建的Upvalue数
        Size upvalues_closed = 0;           // 闭合的Upvalue数
        Size upvalues_collected = 0;        // 回收的Upvalue数
        Size shared_upvalues = 0;           // 共享的Upvalue数
        
        Size memory_usage = 0;              // 内存使用量
        Size peak_upvalue_count = 0;        // 峰值Upvalue数
        double avg_reference_count = 0.0;   // 平均引用计数
        double hit_rate = 0.0;              // 查找命中率
    };
    
    /**
     * @brief 获取统计信息
     */
    const UpvalueStatistics& GetStatistics() const { return statistics_; }
    
    /**
     * @brief 重置统计信息
     */
    void ResetStatistics();
    
    /**
     * @brief 更新统计信息
     */
    void UpdateStatistics();
    
    /* ====================================================================== */
    /* 调试和诊断 */
    /* ====================================================================== */
    
    /**
     * @brief 验证管理器完整性
     */
    struct ValidationResult {
        bool is_valid;
        std::vector<std::string> issues;
        std::vector<std::string> warnings;
        std::vector<std::string> performance_tips;
    };
    
    /**
     * @brief 验证完整性
     */
    ValidationResult ValidateIntegrity() const;
    
    /**
     * @brief 获取调试信息
     */
    std::string GetDebugInfo() const;
    
    /**
     * @brief 获取详细报告
     */
    std::string GenerateReport() const;
    
    /**
     * @brief 导出Upvalue状态
     * @return Upvalue信息列表
     */
    std::vector<Upvalue::UpvalueInfo> ExportUpvalueStates() const;
    
    /**
     * @brief 检查内存泄漏
     */
    bool CheckForMemoryLeaks() const;
    
    /* ====================================================================== */
    /* 配置和优化 */
    /* ====================================================================== */
    
    /**
     * @brief 管理器配置
     */
    struct ManagerConfig {
        bool enable_automatic_cleanup = true;      // 启用自动清理
        Size cleanup_threshold = 100;              // 清理阈值
        bool enable_sharing_optimization = true;   // 启用共享优化
        bool enable_statistics = true;             // 启用统计
        Size max_upvalue_cache_size = 1000;       // 最大缓存大小
    };
    
    /**
     * @brief 设置配置
     */
    void SetConfig(const ManagerConfig& config) { config_ = config; }
    
    /**
     * @brief 获取配置
     */
    const ManagerConfig& GetConfig() const { return config_; }

private:
    /* ====================================================================== */
    /* 内部数据结构 */
    /* ====================================================================== */
    
    /**
     * @brief Upvalue缓存条目
     */
    struct UpvalueCacheEntry {
        std::shared_ptr<Upvalue> upvalue;
        Size last_access_time;
        Size access_count;
    };
    
    /* ====================================================================== */
    /* 内部方法 */
    /* ====================================================================== */
    
    /**
     * @brief 插入Upvalue到有序链表
     * @param upvalue 要插入的Upvalue
     */
    void InsertUpvalueOrdered(std::shared_ptr<Upvalue> upvalue);
    
    /**
     * @brief 从链表中移除Upvalue
     * @param upvalue 要移除的Upvalue
     */
    void RemoveUpvalueFromList(std::shared_ptr<Upvalue> upvalue);
    
    /**
     * @brief 查找插入位置
     * @param stack_index 栈索引
     * @return 插入位置的迭代器
     */
    auto FindInsertionPoint(Size stack_index);
    
    /**
     * @brief 更新访问统计
     * @param upvalue 访问的Upvalue
     * @param is_hit 是否命中缓存
     */
    void UpdateAccessStatistics(std::shared_ptr<Upvalue> upvalue, bool is_hit);
    
    /**
     * @brief 执行自动清理
     */
    void PerformAutomaticCleanup();
    
    /**
     * @brief 验证Upvalue链表完整性
     */
    bool ValidateUpvalueList() const;
    
    /**
     * @brief 计算当前内存使用
     */
    Size CalculateMemoryUsage() const;
    
    /* ====================================================================== */
    /* 成员变量 */
    /* ====================================================================== */
    
    LuaStack* stack_;                                           // 关联的栈
    
    // Upvalue存储（按栈索引有序）
    std::map<Size, std::shared_ptr<Upvalue>> upvalue_map_;      // 主要存储
    std::shared_ptr<Upvalue> open_upvalue_head_;                // 开放Upvalue链表头
    
    // 缓存和优化
    std::unordered_map<Size, UpvalueCacheEntry> upvalue_cache_; // 访问缓存
    Size cache_access_counter_;                                 // 缓存访问计数
    
    // 统计和配置
    UpvalueStatistics statistics_;                              // 统计信息
    ManagerConfig config_;                                      // 配置
    
    // GC集成
    std::weak_ptr<GarbageCollector> gc_;                       // 垃圾收集器（如果可用）
};

/* ========================================================================== */
/* 工厂函数 */
/* ========================================================================== */

/**
 * @brief 创建标准Upvalue管理器
 */
std::unique_ptr<UpvalueManager> CreateStandardUpvalueManager(LuaStack* stack);

/**
 * @brief 创建高性能Upvalue管理器
 */
std::unique_ptr<UpvalueManager> CreateHighPerformanceUpvalueManager(LuaStack* stack);

/**
 * @brief 创建调试Upvalue管理器
 */
std::unique_ptr<UpvalueManager> CreateDebugUpvalueManager(LuaStack* stack);

} // namespace lua_cpp