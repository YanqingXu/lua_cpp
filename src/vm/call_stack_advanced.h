#pragma once

#include "call_frame.h"
#include "core/lua_common.h"
#include "core/lua_errors.h"
#include <memory>
#include <vector>
#include <map>
#include <chrono>

namespace lua_cpp {

/* ========================================================================== */
/* 前向声明 */
/* ========================================================================== */

class Proto;
class LuaValue;

/* ========================================================================== */
/* 高级调用栈管理器（T026）*/
/* ========================================================================== */

/**
 * @brief 高级调用栈管理器（独立实现，不继承）
 * 
 * T026 增强功能：尾调用优化、性能监控和调试增强
 * 
 * 设计哲学：
 * - 不继承基础 CallStack（避免过度抽象）
 * - 独立管理自己的 CallFrame 数组（组合而非继承）
 * - 提供 T026 特有的高级功能
 * - 可被 EnhancedVirtualMachine 选择性使用
 * 
 * 主要特性：
 * - 尾调用优化（避免栈溢出）
 * - 性能统计和监控
 * - 增强的调试信息
 * - 调用模式分析
 */
class AdvancedCallStackManager {
public:
    /**
     * @brief 构造函数
     * @param max_depth 最大调用深度
     */
    explicit AdvancedCallStackManager(Size max_depth = VM_MAX_CALL_STACK_DEPTH);
    
    /**
     * @brief 析构函数
     */
    ~AdvancedCallStackManager() = default;
    
    // 禁用拷贝，允许移动
    AdvancedCallStackManager(const AdvancedCallStackManager&) = delete;
    AdvancedCallStackManager& operator=(const AdvancedCallStackManager&) = delete;
    AdvancedCallStackManager(AdvancedCallStackManager&&) = default;
    AdvancedCallStackManager& operator=(AdvancedCallStackManager&&) = default;
    
    /* ====================================================================== */
    /* 基础调用栈操作（独立实现，不依赖继承）*/
    /* ====================================================================== */
    
    /**
     * @brief 推入新的调用帧
     * @param proto 函数原型
     * @param base 堆栈基址
     * @param param_count 参数数量
     * @param return_address 返回地址
     */
    void PushFrame(const Proto* proto, Size base, Size param_count, Size return_address = 0);
    
    /**
     * @brief 弹出当前调用帧
     * @return 弹出的调用帧
     */
    CallFrame PopFrame();
    
    /**
     * @brief 获取当前调用帧
     */
    CallFrame& GetCurrentFrame();
    const CallFrame& GetCurrentFrame() const;
    
    /**
     * @brief 获取调用栈深度
     */
    Size GetDepth() const { return current_frame_index_ + 1; }
    
    /**
     * @brief 获取最大深度
     */
    Size GetMaxDepth() const { return max_depth_; }
    
    /**
     * @brief 检查调用栈是否为空
     */
    bool IsEmpty() const { return current_frame_index_ == 0; }
    
    /**
     * @brief 清空调用栈
     */
    void Clear();
    
    /* ====================================================================== */
    /* 尾调用优化（T026 特有功能）*/
    /* ====================================================================== */
    
    /**
     * @brief 检查是否可以进行尾调用优化
     * @param proto 目标函数原型
     * @param param_count 参数数量
     * @return 是否可以优化
     */
    bool CanOptimizeTailCall(const Proto* proto, Size param_count);
    
    /**
     * @brief 执行尾调用优化
     * @param proto 目标函数原型
     * @param param_count 参数数量
     * @param args 参数数组（从栈中移动）
     */
    void ExecuteTailCallOptimization(const Proto* proto, Size param_count, 
                                   const std::vector<LuaValue>& args);
    
    /**
     * @brief 准备尾调用（参数移动和栈调整）
     * @param func_reg 函数寄存器索引
     * @param param_count 参数数量
     */
    void PrepareTailCall(RegisterIndex func_reg, Size param_count);
    
    /**
     * @brief 检测递归模式
     * @param proto 函数原型
     * @return 是否为递归调用
     */
    bool IsRecursiveCall(const Proto* proto) const;
    
    /**
     * @brief 获取递归深度
     * @param proto 函数原型
     * @return 递归深度（0表示非递归）
     */
    Size GetRecursionDepth(const Proto* proto) const;
    
    /* ====================================================================== */
    /* 性能监控 */
    /* ====================================================================== */
    
    /**
     * @brief 调用栈性能指标
     */
    struct CallStackMetrics {
        // 尾调用统计
        Size tail_calls_attempted = 0;        // 尝试的尾调用数
        Size tail_calls_optimized = 0;        // 成功优化的尾调用数
        Size tail_call_depth_saved = 0;       // 节省的调用深度总计
        
        // 调用深度统计
        Size max_depth_reached = 0;           // 达到的最大深度
        Size current_depth = 0;               // 当前深度
        double avg_call_depth = 0.0;          // 平均调用深度
        
        // 递归统计
        Size recursive_calls = 0;             // 递归调用次数
        Size max_recursion_depth = 0;         // 最大递归深度
        Size deep_recursion_count = 0;        // 深度递归次数(>100)
        
        // 性能统计
        Size total_function_calls = 0;        // 总函数调用次数
        Size total_function_returns = 0;      // 总函数返回次数
        double avg_call_duration = 0.0;       // 平均调用持续时间(ms)
        std::chrono::steady_clock::time_point measurement_start; // 测量开始时间
        
        // 内存统计
        Size peak_memory_usage = 0;           // 峰值内存使用(字节)
        Size current_memory_usage = 0;        // 当前内存使用(字节)
        Size memory_saves_from_tail_calls = 0; // 尾调用节省的内存
    };
    
    /**
     * @brief 获取性能指标
     */
    const CallStackMetrics& GetMetrics() const { return metrics_; }
    
    /**
     * @brief 重置性能指标
     */
    void ResetMetrics();
    
    /**
     * @brief 更新调用时间统计
     * @param call_start_time 调用开始时间
     */
    void UpdateCallTiming(std::chrono::steady_clock::time_point call_start_time);
    
    /**
     * @brief 更新内存使用统计
     * @param current_usage 当前内存使用量
     */
    void UpdateMemoryUsage(Size current_usage);
    
    /* ====================================================================== */
    /* 调用模式分析 */
    /* ====================================================================== */
    
    /**
     * @brief 调用模式类型
     */
    enum class CallPattern {
        NORMAL,           // 普通调用
        TAIL_RECURSIVE,   // 尾递归
        MUTUAL_RECURSIVE, // 互相递归
        DEEP_RECURSIVE,   // 深度递归
        ITERATIVE,        // 迭代模式
        UNKNOWN           // 未知模式
    };
    
    /**
     * @brief 分析当前调用模式
     * @return 调用模式类型
     */
    CallPattern AnalyzeCallPattern() const;
    
    /**
     * @brief 获取调用模式统计
     */
    std::map<CallPattern, Size> GetCallPatternStats() const;
    
    /**
     * @brief 建议优化策略
     * @param pattern 调用模式
     * @return 优化建议字符串
     */
    std::string GetOptimizationSuggestion(CallPattern pattern) const;
    
    /* ====================================================================== */
    /* 调试增强 */
    /* ====================================================================== */
    
    /**
     * @brief 获取详细的堆栈跟踪
     * @param include_registers 是否包含寄存器信息
     * @param include_locals 是否包含局部变量信息
     * @return 详细堆栈跟踪字符串
     */
    std::string GetDetailedStackTrace(bool include_registers = false,
                                    bool include_locals = false) const;
    
    /**
     * @brief 获取函数调用链
     * @return 函数名称列表（从最深层到最外层）
     */
    std::vector<std::string> GetFunctionCallChain() const;
    
    /**
     * @brief 获取调用图信息（用于可视化）
     */
    struct CallGraphNode {
        std::string function_name;
        Size call_count;
        double total_time;
        std::vector<std::shared_ptr<CallGraphNode>> children;
    };
    
    /**
     * @brief 构建调用图
     * @return 调用图根节点
     */
    std::shared_ptr<CallGraphNode> BuildCallGraph() const;
    
    /**
     * @brief 导出调用图为DOT格式
     * @return DOT格式字符串
     */
    std::string ExportCallGraphToDot() const;
    
    /* ====================================================================== */
    /* 重写基类方法以添加统计 */
    /* ====================================================================== */
    
    /**
     * @brief 推入调用帧（带统计）
     */
    void PushFrame(const Proto* proto, Size base, Size param_count, 
                   Size return_address = 0);
    
    /**
     * @brief 弹出调用帧（带统计）
     */
    CallFrame PopFrame();
    
    /**
     * @brief 清空调用栈（带统计重置）
     */
    void Clear();
    
    /* ====================================================================== */
    /* 调用栈验证和诊断 */
    /* ====================================================================== */
    
    /**
     * @brief 验证调用栈完整性（增强版）
     * @return 验证结果和详细信息
     */
    struct ValidationResult {
        bool is_valid;
        std::vector<std::string> issues;
        std::vector<std::string> warnings;
        std::vector<std::string> suggestions;
    };
    
    /**
     * @brief 执行完整性验证
     */
    ValidationResult ValidateIntegrityAdvanced() const;
    
    /**
     * @brief 诊断调用栈问题
     * @return 诊断报告
     */
    std::string DiagnoseCallStackIssues() const;
    
    /**
     * @brief 生成性能报告
     * @return 性能分析报告
     */
    std::string GeneratePerformanceReport() const;

private:
    /* ====================================================================== */
    /* 内部方法 */
    /* ====================================================================== */
    
    /**
     * @brief 更新调用模式统计
     * @param pattern 检测到的模式
     */
    void UpdateCallPatternStats(CallPattern pattern);
    
    /**
     * @brief 检查尾调用的前置条件
     * @param proto 目标函数
     * @return 是否满足尾调用条件
     */
    bool CheckTailCallPreconditions(const Proto* proto) const;
    
    /**
     * @brief 计算预期的内存节省
     * @param avoided_frames 避免创建的帧数
     * @return 节省的内存字节数
     */
    Size CalculateMemorySavings(Size avoided_frames) const;
    
    /**
     * @brief 记录函数调用开始
     * @param proto 函数原型
     */
    void RecordCallStart(const Proto* proto);
    
    /**
     * @brief 记录函数调用结束
     * @param proto 函数原型
     */
    void RecordCallEnd(const Proto* proto);
    
    /* ====================================================================== */
    /* 成员变量 */
    /* ====================================================================== */
    
    // 基础调用栈数据（Lua 5.1.5 风格）
    std::vector<CallFrame> frames_;         // 调用帧数组
    Size current_frame_index_;              // 当前帧索引
    Size max_depth_;                        // 最大深度
    
    // T026 性能统计
    CallStackMetrics metrics_;
    
    // 调用模式统计
    std::map<CallPattern, Size> pattern_stats_;
    
    // 调用计时
    std::map<const Proto*, std::chrono::steady_clock::time_point> call_start_times_;
    
    // 递归检测
    std::map<const Proto*, Size> recursion_depths_;
    
    // 调用历史（用于模式分析）
    std::vector<const Proto*> call_history_;
    static constexpr Size MAX_CALL_HISTORY = 1000;
    
    // 内存使用跟踪
    Size frame_memory_overhead_;
};

/* ========================================================================== */
/* 工厂函数 */
/* ========================================================================== */

/**
 * @brief 创建标准高级调用栈管理器
 */
std::unique_ptr<AdvancedCallStackManager> CreateStandardAdvancedCallStack();

/**
 * @brief 创建高性能调用栈管理器（减少统计开销）
 */
std::unique_ptr<AdvancedCallStackManager> CreateHighPerformanceCallStack();

/**
 * @brief 创建调试调用栈管理器（最详细的统计和跟踪）
 */
std::unique_ptr<AdvancedCallStackManager> CreateDebugCallStack();

} // namespace lua_cpp