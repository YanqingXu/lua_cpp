#pragma once

#include "virtual_machine.h"
#include "call_stack_advanced.h"
#include "upvalue_manager.h"
#include "coroutine_support.h"
#include "../stdlib/stdlib.h"
#include "core/common.h"
#include "core/lua_value.h"
#include <memory>

namespace lua_cpp {

/**
 * @brief T026增强虚拟机
 * 
 * 集成T026高级调用栈管理功能到现有VM系统
 * 提供向后兼容性并增强功能
 */
class EnhancedVirtualMachine : public VirtualMachine {
public:
    /**
     * @brief 构造函数
     * @param config VM配置
     */
    explicit EnhancedVirtualMachine(const VMConfig& config = VMConfig());
    
    /**
     * @brief 析构函数
     */
    ~EnhancedVirtualMachine() override = default;
    
    // 禁用拷贝，允许移动
    EnhancedVirtualMachine(const EnhancedVirtualMachine&) = delete;
    EnhancedVirtualMachine& operator=(const EnhancedVirtualMachine&) = delete;
    EnhancedVirtualMachine(EnhancedVirtualMachine&&) = default;
    EnhancedVirtualMachine& operator=(EnhancedVirtualMachine&&) = default;
    
    /* ====================================================================== */
    /* T026组件访问 */
    /* ====================================================================== */
    
    /**
     * @brief 获取高级调用栈
     */
    AdvancedCallStack& GetAdvancedCallStack() { return *advanced_call_stack_; }
    const AdvancedCallStack& GetAdvancedCallStack() const { return *advanced_call_stack_; }
    
    /**
     * @brief 获取Upvalue管理器
     */
    UpvalueManager& GetUpvalueManager() { return *upvalue_manager_; }
    const UpvalueManager& GetUpvalueManager() const { return *upvalue_manager_; }
    
    /**
     * @brief 获取协程支持
     */
    CoroutineSupport& GetCoroutineSupport() { return *coroutine_support_; }
    const CoroutineSupport& GetCoroutineSupport() const { return *coroutine_support_; }
    
    /* ====================================================================== */
    /* 增强执行功能 */
    /* ====================================================================== */
    
    /**
     * @brief 执行程序（使用T026增强功能）
     * @param proto 主函数原型
     * @param args 程序参数
     * @return 程序返回值
     */
    std::vector<LuaValue> ExecuteProgramEnhanced(const Proto* proto, 
                                               const std::vector<LuaValue>& args = {}) override;
    
    /**
     * @brief 创建协程并返回协程对象
     * @param func 协程函数
     * @param args 初始参数
     * @return 协程对象
     */
    LuaValue CreateCoroutine(const LuaValue& func, const std::vector<LuaValue>& args = {});
    
    /**
     * @brief 恢复协程执行
     * @param coroutine 协程对象
     * @param args resume参数
     * @return 执行结果
     */
    std::vector<LuaValue> ResumeCoroutine(const LuaValue& coroutine, 
                                        const std::vector<LuaValue>& args = {});
    
    /**
     * @brief 挂起当前协程
     * @param yield_values yield值
     * @return resume时传入的参数
     */
    std::vector<LuaValue> YieldCoroutine(const std::vector<LuaValue>& yield_values = {});
    
    /* ====================================================================== */
    /* 增强调试功能 */
    /* ====================================================================== */
    
    /**
     * @brief 获取增强的调用栈跟踪
     * @return 详细的堆栈跟踪信息
     */
    std::string GetEnhancedStackTrace() const;
    
    /**
     * @brief 获取性能报告
     * @return 性能分析报告
     */
    std::string GetPerformanceReport() const;
    
    /**
     * @brief 获取调用模式分析
     * @return 调用模式分析结果
     */
    std::string GetCallPatternAnalysis() const;
    
    /**
     * @brief 获取Upvalue使用统计
     * @return Upvalue统计信息
     */
    std::string GetUpvalueStatistics() const;
    
    /**
     * @brief 获取协程状态概览
     * @return 协程状态信息
     */
    std::string GetCoroutineOverview() const;
    
    /* ====================================================================== */
    /* 配置管理 */
    /* ====================================================================== */
    
    /**
     * @brief T026配置结构
     */
    struct T026Config {
        // 高级调用栈配置
        bool enable_tail_call_optimization = true;     // 启用尾调用优化
        bool enable_performance_monitoring = true;     // 启用性能监控
        bool enable_call_pattern_analysis = false;     // 启用调用模式分析
        
        // Upvalue配置
        bool enable_upvalue_caching = true;            // 启用Upvalue缓存
        bool enable_upvalue_sharing = true;            // 启用Upvalue共享
        bool enable_gc_integration = true;             // 启用GC集成
        
        // 协程配置
        bool enable_coroutine_support = false;         // 启用协程支持
        CoroutineScheduler::SchedulingPolicy 
            coroutine_scheduling = CoroutineScheduler::SchedulingPolicy::COOPERATIVE;
        Size max_coroutines = 100;                     // 最大协程数
        Size coroutine_stack_size = 256;              // 协程栈大小
    };
    
    /**
     * @brief 设置T026配置
     */
    void SetT026Config(const T026Config& config);
    
    /**
     * @brief 获取T026配置
     */
    const T026Config& GetT026Config() const { return t026_config_; }
    
    /* ====================================================================== */
    /* 兼容性接口 */
    /* ====================================================================== */
    
    /**
     * @brief 检查是否启用T026功能
     */
    bool IsT026Enabled() const { return t026_enabled_; }
    
    /**
     * @brief 启用/禁用T026功能
     * @param enabled 是否启用
     */
    void SetT026Enabled(bool enabled) { t026_enabled_ = enabled; }
    
    /**
     * @brief 获取传统调用栈（向后兼容）
     */
    const std::vector<CallFrame>& GetLegacyCallStack() const;
    
    /**
     * @brief 切换到传统模式
     */
    void SwitchToLegacyMode();
    
    /**
     * @brief 切换到增强模式
     */
    void SwitchToEnhancedMode();
    
    /* ====================================================================== */
    /* T027标准库接口 */
    /* ====================================================================== */
    
    /**
     * @brief 初始化标准库
     */
    void InitializeStandardLibrary();
    
    /**
     * @brief 获取标准库实例
     */
    StandardLibrary* GetStandardLibrary() const { return standard_library_.get(); }

protected:
    /* ====================================================================== */
    /* 重写的VM方法 */
    /* ====================================================================== */
    
    /**
     * @brief 执行函数调用指令（增强版）
     */
    void ExecuteCALLEnhanced(RegisterIndex a, int b, int c) override;
    
    /**
     * @brief 执行尾调用指令（增强版）
     */
    void ExecuteTAILCALLEnhanced(RegisterIndex a, int b, int c) override;
    
    /**
     * @brief 执行返回指令（增强版）
     */
    void ExecuteRETURNEnhanced(RegisterIndex a, int b) override;
    
    /**
     * @brief 执行闭包指令（增强版）
     */
    void ExecuteCLOSUREEnhanced(RegisterIndex a, int bx) override;
    
private:
    /* ====================================================================== */
    /* 内部方法 */
    /* ====================================================================== */
    
    /**
     * @brief 初始化T026组件
     */
    void InitializeT026Components();
    
    /**
     * @brief 同步调用栈状态
     */
    void SyncCallStackState();
    
    /**
     * @brief 处理Upvalue创建
     */
    std::shared_ptr<Upvalue> CreateUpvalue(Size stack_index);
    
    /**
     * @brief 处理Upvalue关闭
     */
    void CloseUpvalues(Size level);
    
    /**
     * @brief 设置尾调用标记
     */
    void SetTailCallFlag(bool is_tail_call);
    
    /**
     * @brief 检查是否应该优化尾调用
     */
    bool ShouldOptimizeTailCall() const;
    
    /**
     * @brief 更新性能统计
     */
    void UpdatePerformanceStats();
    
    /* ====================================================================== */
    /* 成员变量 */
    /* ====================================================================== */
    
    // T026组件
    std::unique_ptr<AdvancedCallStack> advanced_call_stack_;
    std::unique_ptr<UpvalueManager> upvalue_manager_;
    std::unique_ptr<CoroutineSupport> coroutine_support_;
    
    // T027标准库组件
    std::unique_ptr<StandardLibrary> standard_library_;
    
    // 配置和状态
    T026Config t026_config_;
    bool t026_enabled_;
    bool is_tail_call_;
    
    // 兼容性支持
    std::vector<CallFrame> legacy_call_stack_;
    bool legacy_mode_;
};

/* ========================================================================== */
/* VM适配器 */
/* ========================================================================== */

/**
 * @brief VM适配器类
 * 
 * 为现有代码提供平滑的迁移路径
 * 允许逐步采用T026功能
 */
class VirtualMachineAdapter {
public:
    /**
     * @brief 构造函数
     * @param vm 增强虚拟机实例
     */
    explicit VirtualMachineAdapter(std::unique_ptr<EnhancedVirtualMachine> vm);
    
    /**
     * @brief 析构函数
     */
    ~VirtualMachineAdapter() = default;
    
    // 禁用拷贝，允许移动
    VirtualMachineAdapter(const VirtualMachineAdapter&) = delete;
    VirtualMachineAdapter& operator=(const VirtualMachineAdapter&) = delete;
    VirtualMachineAdapter(VirtualMachineAdapter&&) = default;
    VirtualMachineAdapter& operator=(VirtualMachineAdapter&&) = default;
    
    /* ====================================================================== */
    /* 传统接口 */
    /* ====================================================================== */
    
    /**
     * @brief 获取传统VM接口
     */
    VirtualMachine& GetLegacyVM() { return *vm_; }
    const VirtualMachine& GetLegacyVM() const { return *vm_; }
    
    /**
     * @brief 获取增强VM接口
     */
    EnhancedVirtualMachine& GetEnhancedVM() { return *vm_; }
    const EnhancedVirtualMachine& GetEnhancedVM() const { return *vm_; }
    
    /* ====================================================================== */
    /* 功能切换 */
    /* ====================================================================== */
    
    /**
     * @brief 启用尾调用优化
     */
    void EnableTailCallOptimization(bool enable = true);
    
    /**
     * @brief 启用性能监控
     */
    void EnablePerformanceMonitoring(bool enable = true);
    
    /**
     * @brief 启用协程支持
     */
    void EnableCoroutineSupport(bool enable = true);
    
    /**
     * @brief 启用Upvalue管理
     */
    void EnableUpvalueManagement(bool enable = true);
    
    /* ====================================================================== */
    /* 迁移工具 */
    /* ====================================================================== */
    
    /**
     * @brief 分析现有代码的兼容性
     * @return 兼容性报告
     */
    std::string AnalyzeCompatibility() const;
    
    /**
     * @brief 获取迁移建议
     * @return 迁移建议列表
     */
    std::vector<std::string> GetMigrationSuggestions() const;
    
    /**
     * @brief 执行性能对比测试
     * @param legacy_runs 传统模式运行次数
     * @param enhanced_runs 增强模式运行次数
     * @return 性能对比报告
     */
    std::string RunPerformanceComparison(Size legacy_runs, Size enhanced_runs) const;

private:
    std::unique_ptr<EnhancedVirtualMachine> vm_;
};

/* ========================================================================== */
/* 工厂函数 */
/* ========================================================================== */

/**
 * @brief 创建增强虚拟机（启用所有T026功能）
 */
std::unique_ptr<EnhancedVirtualMachine> CreateEnhancedVM();

/**
 * @brief 创建兼容虚拟机（保持传统行为）
 */
std::unique_ptr<EnhancedVirtualMachine> CreateCompatibleVM();

/**
 * @brief 创建高性能虚拟机（启用性能优化）
 */
std::unique_ptr<EnhancedVirtualMachine> CreateHighPerformanceEnhancedVM();

/**
 * @brief 创建调试增强虚拟机（启用详细诊断）
 */
std::unique_ptr<EnhancedVirtualMachine> CreateDebugEnhancedVM();

/**
 * @brief 从传统VM升级到增强VM
 * @param legacy_vm 传统VM实例
 * @return 升级后的增强VM
 */
std::unique_ptr<EnhancedVirtualMachine> UpgradeToEnhancedVM(
    std::unique_ptr<VirtualMachine> legacy_vm);

/**
 * @brief 创建VM适配器
 * @param config T026配置
 * @return VM适配器实例
 */
std::unique_ptr<VirtualMachineAdapter> CreateVMAdapter(
    const EnhancedVirtualMachine::T026Config& config = {});

} // namespace lua_cpp