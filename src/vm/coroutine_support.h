#pragma once

#include "call_stack_advanced.h"
#include "upvalue_manager.h"
#include "stack.h"
#include "core/lua_common.h"
#include "types/value.h"
#include "core/lua_errors.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>

namespace lua_cpp {

/* ========================================================================== */
/* 前向声明 */
/* ========================================================================== */

class VirtualMachine;
class Proto;

/* ========================================================================== */
/* 协程错误类型 */
/* ========================================================================== */

/**
 * @brief 协程相关错误
 */
class CoroutineError : public LuaError {
public:
    explicit CoroutineError(const std::string& message = "Coroutine error")
        : LuaError(message, ErrorType::RUNTIME_ERROR) {}
};

/**
 * @brief 协程状态错误
 */
class CoroutineStateError : public CoroutineError {
public:
    explicit CoroutineStateError(const std::string& message = "Invalid coroutine state")
        : CoroutineError(message) {}
};

/**
 * @brief 协程切换错误
 */
class CoroutineSwitchError : public CoroutineError {
public:
    explicit CoroutineSwitchError(const std::string& message = "Coroutine switch failed")
        : CoroutineError(message) {}
};

/* ========================================================================== */
/* 协程状态枚举 */
/* ========================================================================== */

/**
 * @brief 协程状态
 */
enum class CoroutineState {
    SUSPENDED,      // 挂起状态（可以resume）
    RUNNING,        // 运行状态（当前执行）
    NORMAL,         // 正常状态（不是主线程，但也不是当前协程）
    DEAD           // 死亡状态（已结束，不能resume）
};

/**
 * @brief 协程状态转换为字符串
 */
std::string CoroutineStateToString(CoroutineState state);

/* ========================================================================== */
/* 协程上下文 */
/* ========================================================================== */

/**
 * @brief 协程执行上下文
 * 
 * 保存协程的完整执行状态，包括：
 * - 调用栈
 * - Lua值栈
 * - Upvalue管理器
 * - 执行状态
 * - 调试信息
 */
class CoroutineContext {
public:
    /**
     * @brief 构造函数
     * @param initial_stack_size 初始栈大小
     * @param max_call_depth 最大调用深度
     */
    CoroutineContext(Size initial_stack_size = 256, Size max_call_depth = 200);
    
    /**
     * @brief 析构函数
     */
    ~CoroutineContext() = default;
    
    // 禁用拷贝，允许移动
    CoroutineContext(const CoroutineContext&) = delete;
    CoroutineContext& operator=(const CoroutineContext&) = delete;
    CoroutineContext(CoroutineContext&&) = default;
    CoroutineContext& operator=(CoroutineContext&&) = default;
    
    /* ====================================================================== */
    /* 状态管理 */
    /* ====================================================================== */
    
    /**
     * @brief 获取协程状态
     */
    CoroutineState GetState() const { return state_; }
    
    /**
     * @brief 设置协程状态
     */
    void SetState(CoroutineState state) { state_ = state; }
    
    /**
     * @brief 检查是否可以恢复
     */
    bool CanResume() const { return state_ == CoroutineState::SUSPENDED; }
    
    /**
     * @brief 检查是否可以挂起
     */
    bool CanYield() const { return state_ == CoroutineState::RUNNING; }
    
    /**
     * @brief 检查是否已死亡
     */
    bool IsDead() const { return state_ == CoroutineState::DEAD; }
    
    /**
     * @brief 检查是否正在运行
     */
    bool IsRunning() const { return state_ == CoroutineState::RUNNING; }
    
    /* ====================================================================== */
    /* 执行上下文访问 */
    /* ====================================================================== */
    
    /**
     * @brief 获取调用栈
     */
    AdvancedCallStack& GetCallStack() { return *call_stack_; }
    const AdvancedCallStack& GetCallStack() const { return *call_stack_; }
    
    /**
     * @brief 获取Lua栈
     */
    LuaStack& GetLuaStack() { return *lua_stack_; }
    const LuaStack& GetLuaStack() const { return *lua_stack_; }
    
    /**
     * @brief 获取Upvalue管理器
     */
    UpvalueManager& GetUpvalueManager() { return *upvalue_manager_; }
    const UpvalueManager& GetUpvalueManager() const { return *upvalue_manager_; }
    
    /**
     * @brief 获取指令指针
     */
    Size GetInstructionPointer() const { return instruction_pointer_; }
    
    /**
     * @brief 设置指令指针
     */
    void SetInstructionPointer(Size ip) { instruction_pointer_ = ip; }
    
    /**
     * @brief 获取当前函数原型
     */
    const Proto* GetCurrentProto() const { return current_proto_; }
    
    /**
     * @brief 设置当前函数原型
     */
    void SetCurrentProto(const Proto* proto) { current_proto_ = proto; }
    
    /* ====================================================================== */
    /* 上下文保存和恢复 */
    /* ====================================================================== */
    
    /**
     * @brief 保存上下文到另一个上下文对象
     * @param target 目标上下文
     */
    void SaveContextTo(CoroutineContext& target) const;
    
    /**
     * @brief 从另一个上下文恢复
     * @param source 源上下文
     */
    void RestoreContextFrom(const CoroutineContext& source);
    
    /**
     * @brief 交换上下文
     * @param other 另一个上下文
     */
    void SwapContext(CoroutineContext& other);
    
    /* ====================================================================== */
    /* 协程参数和返回值 */
    /* ====================================================================== */
    
    /**
     * @brief 设置协程参数
     * @param args 参数列表
     */
    void SetArguments(const std::vector<LuaValue>& args);
    
    /**
     * @brief 获取协程参数
     */
    const std::vector<LuaValue>& GetArguments() const { return arguments_; }
    
    /**
     * @brief 设置返回值
     * @param values 返回值列表
     */
    void SetReturnValues(const std::vector<LuaValue>& values);
    
    /**
     * @brief 获取返回值
     */
    const std::vector<LuaValue>& GetReturnValues() const { return return_values_; }
    
    /**
     * @brief 设置yield值
     * @param values yield的值列表
     */
    void SetYieldValues(const std::vector<LuaValue>& values);
    
    /**
     * @brief 获取yield值
     */
    const std::vector<LuaValue>& GetYieldValues() const { return yield_values_; }
    
    /* ====================================================================== */
    /* 统计和诊断 */
    /* ====================================================================== */
    
    /**
     * @brief 协程统计信息
     */
    struct CoroutineStats {
        Size resume_count = 0;          // resume次数
        Size yield_count = 0;           // yield次数
        Size switch_count = 0;          // 切换次数
        double total_run_time = 0.0;    // 总运行时间（秒）
        double avg_run_time = 0.0;      // 平均运行时间（秒）
        Size max_stack_usage = 0;       // 最大栈使用
        Size max_call_depth = 0;        // 最大调用深度
        std::chrono::steady_clock::time_point created_time;  // 创建时间
        std::chrono::steady_clock::time_point last_run_time; // 上次运行时间
    };
    
    /**
     * @brief 获取统计信息
     */
    const CoroutineStats& GetStats() const { return stats_; }
    
    /**
     * @brief 重置统计信息
     */
    void ResetStats();
    
    /**
     * @brief 更新运行时统计
     * @param run_start 运行开始时间
     */
    void UpdateRunTimeStats(std::chrono::steady_clock::time_point run_start);
    
    /**
     * @brief 获取内存使用量
     */
    Size GetMemoryUsage() const;
    
    /**
     * @brief 获取调试信息
     */
    std::string GetDebugInfo() const;
    
    /**
     * @brief 验证上下文完整性
     */
    bool ValidateIntegrity() const;

private:
    /* ====================================================================== */
    /* 成员变量 */
    /* ====================================================================== */
    
    // 协程状态
    CoroutineState state_;
    
    // 执行上下文
    std::unique_ptr<AdvancedCallStack> call_stack_;
    std::unique_ptr<LuaStack> lua_stack_;
    std::unique_ptr<UpvalueManager> upvalue_manager_;
    
    // 执行状态
    Size instruction_pointer_;
    const Proto* current_proto_;
    
    // 协程数据
    std::vector<LuaValue> arguments_;       // 协程参数
    std::vector<LuaValue> return_values_;   // 返回值
    std::vector<LuaValue> yield_values_;    // yield值
    
    // 统计信息
    CoroutineStats stats_;
};

/* ========================================================================== */
/* 协程调度器 */
/* ========================================================================== */

/**
 * @brief 协程调度器
 * 
 * 管理协程的创建、切换、销毁和调度策略
 * 支持协作式调度和优先级调度
 */
class CoroutineScheduler {
public:
    /**
     * @brief 构造函数
     */
    CoroutineScheduler();
    
    /**
     * @brief 析构函数
     */
    ~CoroutineScheduler() = default;
    
    // 禁用拷贝，允许移动
    CoroutineScheduler(const CoroutineScheduler&) = delete;
    CoroutineScheduler& operator=(const CoroutineScheduler&) = delete;
    CoroutineScheduler(CoroutineScheduler&&) = default;
    CoroutineScheduler& operator=(CoroutineScheduler&&) = default;
    
    /* ====================================================================== */
    /* 协程生命周期管理 */
    /* ====================================================================== */
    
    using CoroutineId = Size;
    
    /**
     * @brief 创建新协程
     * @param proto 协程函数原型
     * @param args 初始参数
     * @return 协程ID
     */
    CoroutineId CreateCoroutine(const Proto* proto, const std::vector<LuaValue>& args = {});
    
    /**
     * @brief 销毁协程
     * @param id 协程ID
     */
    void DestroyCoroutine(CoroutineId id);
    
    /**
     * @brief 获取协程上下文
     * @param id 协程ID
     * @return 协程上下文指针
     */
    std::shared_ptr<CoroutineContext> GetCoroutine(CoroutineId id);
    
    /**
     * @brief 获取当前运行的协程ID
     */
    CoroutineId GetCurrentCoroutineId() const { return current_coroutine_id_; }
    
    /**
     * @brief 获取当前运行的协程
     */
    std::shared_ptr<CoroutineContext> GetCurrentCoroutine() const;
    
    /**
     * @brief 检查协程是否存在
     */
    bool CoroutineExists(CoroutineId id) const;
    
    /* ====================================================================== */
    /* 协程调度 */
    /* ====================================================================== */
    
    /**
     * @brief 恢复协程执行
     * @param id 协程ID
     * @param args resume参数
     * @return 协程执行结果（yield值或返回值）
     */
    std::vector<LuaValue> ResumeCoroutine(CoroutineId id, const std::vector<LuaValue>& args = {});
    
    /**
     * @brief 挂起当前协程
     * @param yield_values yield的值
     * @return 下次resume时传入的参数
     */
    std::vector<LuaValue> YieldCoroutine(const std::vector<LuaValue>& yield_values = {});
    
    /**
     * @brief 切换到指定协程
     * @param id 目标协程ID
     */
    void SwitchToCoroutine(CoroutineId id);
    
    /**
     * @brief 切换回主线程
     */
    void SwitchToMainThread();
    
    /* ====================================================================== */
    /* 调度策略 */
    /* ====================================================================== */
    
    /**
     * @brief 调度策略类型
     */
    enum class SchedulingPolicy {
        COOPERATIVE,    // 协作式调度（手动yield/resume）
        PREEMPTIVE,     // 抢占式调度（时间片轮转）
        PRIORITY        // 优先级调度
    };
    
    /**
     * @brief 设置调度策略
     */
    void SetSchedulingPolicy(SchedulingPolicy policy) { scheduling_policy_ = policy; }
    
    /**
     * @brief 获取调度策略
     */
    SchedulingPolicy GetSchedulingPolicy() const { return scheduling_policy_; }
    
    /**
     * @brief 设置协程优先级（优先级调度时使用）
     * @param id 协程ID
     * @param priority 优先级（越小优先级越高）
     */
    void SetCoroutinePriority(CoroutineId id, int priority);
    
    /**
     * @brief 获取协程优先级
     */
    int GetCoroutinePriority(CoroutineId id) const;
    
    /* ====================================================================== */
    /* 批量操作 */
    /* ====================================================================== */
    
    /**
     * @brief 获取所有协程ID
     */
    std::vector<CoroutineId> GetAllCoroutineIds() const;
    
    /**
     * @brief 获取活跃协程数量
     */
    Size GetActiveCoroutineCount() const;
    
    /**
     * @brief 清理已死亡的协程
     * @return 清理的数量
     */
    Size CleanupDeadCoroutines();
    
    /**
     * @brief 暂停所有协程
     */
    void SuspendAllCoroutines();
    
    /**
     * @brief 销毁所有协程
     */
    void DestroyAllCoroutines();
    
    /* ====================================================================== */
    /* 统计和监控 */
    /* ====================================================================== */
    
    /**
     * @brief 调度器统计信息
     */
    struct SchedulerStats {
        Size total_coroutines_created = 0;      // 创建的协程总数
        Size total_coroutines_destroyed = 0;    // 销毁的协程总数
        Size current_coroutine_count = 0;       // 当前协程数量
        Size total_context_switches = 0;        // 总上下文切换次数
        Size total_resumes = 0;                 // 总resume次数
        Size total_yields = 0;                  // 总yield次数
        double avg_switch_time = 0.0;           // 平均切换时间（微秒）
        Size max_concurrent_coroutines = 0;     // 最大并发协程数
        Size memory_usage = 0;                  // 内存使用量
    };
    
    /**
     * @brief 获取统计信息
     */
    const SchedulerStats& GetStats() const { return stats_; }
    
    /**
     * @brief 重置统计信息
     */
    void ResetStats();
    
    /**
     * @brief 更新统计信息
     */
    void UpdateStats();
    
    /**
     * @brief 获取调度器状态报告
     */
    std::string GetStatusReport() const;
    
    /* ====================================================================== */
    /* 调试和诊断 */
    /* ====================================================================== */
    
    /**
     * @brief 获取协程状态概览
     */
    std::string GetCoroutineOverview() const;
    
    /**
     * @brief 获取详细调试信息
     */
    std::string GetDebugInfo() const;
    
    /**
     * @brief 验证调度器完整性
     */
    bool ValidateIntegrity() const;
    
    /**
     * @brief 检查死锁
     */
    bool CheckForDeadlock() const;

private:
    /* ====================================================================== */
    /* 内部数据结构 */
    /* ====================================================================== */
    
    /**
     * @brief 协程条目
     */
    struct CoroutineEntry {
        std::shared_ptr<CoroutineContext> context;
        int priority;
        std::chrono::steady_clock::time_point last_run_time;
        Size total_run_count;
    };
    
    /* ====================================================================== */
    /* 内部方法 */
    /* ====================================================================== */
    
    /**
     * @brief 生成新的协程ID
     */
    CoroutineId GenerateCoroutineId();
    
    /**
     * @brief 执行上下文切换
     * @param from_id 源协程ID
     * @param to_id 目标协程ID
     */
    void PerformContextSwitch(CoroutineId from_id, CoroutineId to_id);
    
    /**
     * @brief 选择下一个要运行的协程
     */
    CoroutineId SelectNextCoroutine() const;
    
    /**
     * @brief 更新切换时间统计
     * @param switch_start 切换开始时间
     */
    void UpdateSwitchTimeStats(std::chrono::steady_clock::time_point switch_start);
    
    /* ====================================================================== */
    /* 成员变量 */
    /* ====================================================================== */
    
    // 协程存储
    std::unordered_map<CoroutineId, CoroutineEntry> coroutines_;
    CoroutineId next_coroutine_id_;
    CoroutineId current_coroutine_id_;
    
    // 主线程上下文（用于切换回主线程）
    std::unique_ptr<CoroutineContext> main_thread_context_;
    
    // 调度策略
    SchedulingPolicy scheduling_policy_;
    
    // 统计信息
    SchedulerStats stats_;
};

/* ========================================================================== */
/* 协程支持类 */
/* ========================================================================== */

/**
 * @brief 协程支持系统
 * 
 * 为虚拟机提供协程功能的高级接口
 * 整合协程调度器和VM执行器
 */
class CoroutineSupport {
public:
    /**
     * @brief 构造函数
     * @param vm 关联的虚拟机
     */
    explicit CoroutineSupport(VirtualMachine* vm);
    
    /**
     * @brief 析构函数
     */
    ~CoroutineSupport() = default;
    
    // 禁用拷贝，允许移动
    CoroutineSupport(const CoroutineSupport&) = delete;
    CoroutineSupport& operator=(const CoroutineSupport&) = delete;
    CoroutineSupport(CoroutineSupport&&) = default;
    CoroutineSupport& operator=(CoroutineSupport&&) = default;
    
    /* ====================================================================== */
    /* 协程操作接口 */
    /* ====================================================================== */
    
    /**
     * @brief 创建协程
     * @param func 协程函数
     * @param args 初始参数
     * @return 协程对象
     */
    LuaValue CreateCoroutine(const LuaValue& func, const std::vector<LuaValue>& args = {});
    
    /**
     * @brief 恢复协程
     * @param coroutine 协程对象
     * @param args resume参数
     * @return 执行结果
     */
    std::vector<LuaValue> Resume(const LuaValue& coroutine, const std::vector<LuaValue>& args = {});
    
    /**
     * @brief 挂起当前协程
     * @param yield_values yield值
     * @return resume时传入的参数
     */
    std::vector<LuaValue> Yield(const std::vector<LuaValue>& yield_values = {});
    
    /**
     * @brief 获取协程状态
     * @param coroutine 协程对象
     * @return 协程状态字符串
     */
    std::string GetCoroutineStatus(const LuaValue& coroutine);
    
    /**
     * @brief 检查是否在协程中
     */
    bool IsInCoroutine() const;
    
    /**
     * @brief 获取当前运行的协程
     */
    LuaValue GetRunningCoroutine() const;
    
    /* ====================================================================== */
    /* 调度器访问 */
    /* ====================================================================== */
    
    /**
     * @brief 获取调度器
     */
    CoroutineScheduler& GetScheduler() { return scheduler_; }
    const CoroutineScheduler& GetScheduler() const { return scheduler_; }
    
    /**
     * @brief 设置调度策略
     */
    void SetSchedulingPolicy(CoroutineScheduler::SchedulingPolicy policy);
    
    /**
     * @brief 清理资源
     */
    void Cleanup();
    
    /* ====================================================================== */
    /* 配置和优化 */
    /* ====================================================================== */
    
    /**
     * @brief 协程支持配置
     */
    struct CoroutineConfig {
        Size max_coroutines = 1000;                    // 最大协程数量
        Size default_stack_size = 256;                 // 默认栈大小
        Size default_call_depth = 200;                 // 默认调用深度
        bool enable_preemption = false;                // 启用抢占式调度
        Size time_slice_ms = 10;                       // 时间片（毫秒）
        bool enable_priority_scheduling = false;       // 启用优先级调度
        bool enable_statistics = true;                 // 启用统计
        bool enable_gc_integration = true;             // 启用GC集成
    };
    
    /**
     * @brief 设置配置
     */
    void SetConfig(const CoroutineConfig& config) { config_ = config; }
    
    /**
     * @brief 获取配置
     */
    const CoroutineConfig& GetConfig() const { return config_; }
    
    /**
     * @brief 获取统计报告
     */
    std::string GetStatisticsReport() const;

private:
    /* ====================================================================== */
    /* 内部方法 */
    /* ====================================================================== */
    
    /**
     * @brief 协程ID到LuaValue的转换
     */
    LuaValue CoroutineIdToLuaValue(CoroutineScheduler::CoroutineId id) const;
    
    /**
     * @brief LuaValue到协程ID的转换
     */
    CoroutineScheduler::CoroutineId LuaValueToCoroutineId(const LuaValue& value) const;
    
    /**
     * @brief 验证协程对象
     */
    bool IsValidCoroutine(const LuaValue& value) const;
    
    /* ====================================================================== */
    /* 成员变量 */
    /* ====================================================================== */
    
    VirtualMachine* vm_;                    // 关联的虚拟机
    CoroutineScheduler scheduler_;          // 协程调度器
    CoroutineConfig config_;               // 配置
    
    // 协程对象映射（LuaValue到协程ID）
    std::unordered_map<Size, CoroutineScheduler::CoroutineId> coroutine_map_;
    Size next_coroutine_handle_;
};

/* ========================================================================== */
/* 工厂函数 */
/* ========================================================================== */

/**
 * @brief 创建标准协程支持
 */
std::unique_ptr<CoroutineSupport> CreateStandardCoroutineSupport(VirtualMachine* vm);

/**
 * @brief 创建高性能协程支持
 */
std::unique_ptr<CoroutineSupport> CreateHighPerformanceCoroutineSupport(VirtualMachine* vm);

/**
 * @brief 创建调试协程支持
 */
std::unique_ptr<CoroutineSupport> CreateDebugCoroutineSupport(VirtualMachine* vm);

} // namespace lua_cpp