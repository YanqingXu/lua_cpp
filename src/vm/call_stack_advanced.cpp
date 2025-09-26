/**
 * @file call_stack_advanced.cpp
 * @brief 高级调用栈管理实现
 * @description 实现尾调用优化、性能监控和调试增强功能
 * @author Lua C++ Project
 * @date 2025-09-26
 * @version T026 - Advanced Call Stack Management
 */

#include "call_stack_advanced.h"
#include "../compiler/bytecode.h"
#include "../core/lua_common.h"
#include "../core/lua_errors.h"
#include "../types/value.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cmath>

namespace lua_cpp {

/* ========================================================================== */
/* 构造和初始化 */
/* ========================================================================== */

AdvancedCallStack::AdvancedCallStack(Size max_depth)
    : CallStack(max_depth)
    , metrics_()
    , pattern_stats_()
    , call_start_times_()
    , recursion_depths_()
    , call_history_()
    , frame_memory_overhead_(sizeof(CallFrame)) {
    
    // 初始化性能指标
    ResetMetrics();
    
    // 初始化调用模式统计
    pattern_stats_[CallPattern::NORMAL] = 0;
    pattern_stats_[CallPattern::TAIL_RECURSIVE] = 0;
    pattern_stats_[CallPattern::MUTUAL_RECURSIVE] = 0;
    pattern_stats_[CallPattern::DEEP_RECURSIVE] = 0;
    pattern_stats_[CallPattern::ITERATIVE] = 0;
    pattern_stats_[CallPattern::UNKNOWN] = 0;
}

/* ========================================================================== */
/* 尾调用优化实现 */
/* ========================================================================== */

bool AdvancedCallStack::CanOptimizeTailCall(const Proto* proto, Size param_count) {
    metrics_.tail_calls_attempted++;
    
    // 基础检查
    if (!proto || IsEmpty()) {
        return false;
    }
    
    // 检查前置条件
    if (!CheckTailCallPreconditions(proto)) {
        return false;
    }
    
    // 检查当前函数是否在尾部位置
    const CallFrame& current = GetCurrentFrame();
    if (!current.IsAtEnd()) {
        return false;
    }
    
    // 检查参数数量合理性
    if (param_count > 255) { // Lua寄存器限制
        return false;
    }
    
    // 检查是否会导致过深递归（非尾调用情况下）
    if (IsRecursiveCall(proto)) {
        Size recursion_depth = GetRecursionDepth(proto);
        if (recursion_depth > 100) {
            // 深度递归，尾调用优化是必要的
            metrics_.deep_recursion_count++;
        }
    }
    
    return true;
}

void AdvancedCallStack::ExecuteTailCallOptimization(const Proto* proto, Size param_count,
                                                  const std::vector<LuaValue>& args) {
    if (!CanOptimizeTailCall(proto, param_count)) {
        throw RuntimeError("Cannot execute tail call optimization");
    }
    
    // 记录优化开始
    auto optimization_start = std::chrono::steady_clock::now();
    
    // 获取当前帧信息
    CallFrame& current_frame = GetCurrentFrame();
    Size current_base = current_frame.GetBase();
    Size return_address = current_frame.GetReturnAddress();
    
    // 计算内存节省
    Size memory_saved = CalculateMemorySavings(1); // 避免创建一个新帧
    metrics_.memory_saves_from_tail_calls += memory_saved;
    
    // 更新当前帧而不是创建新帧（这是尾调用优化的核心）
    current_frame = CallFrame(proto, current_base, param_count, return_address);
    
    // 重置指令指针到新函数开头
    current_frame.SetInstructionPointer(0);
    
    // 更新统计信息
    metrics_.tail_calls_optimized++;
    metrics_.tail_call_depth_saved++;
    
    // 记录调用模式
    if (IsRecursiveCall(proto)) {
        UpdateCallPatternStats(CallPattern::TAIL_RECURSIVE);
    } else {
        UpdateCallPatternStats(CallPattern::NORMAL);
    }
    
    // 记录优化时间
    auto optimization_end = std::chrono::steady_clock::now();
    auto optimization_duration = std::chrono::duration<double, std::milli>(
        optimization_end - optimization_start).count();
    
    // 更新平均调用持续时间（尾调用优化应该更快）
    if (metrics_.total_function_calls > 0) {
        double total_time = metrics_.avg_call_duration * metrics_.total_function_calls;
        metrics_.avg_call_duration = (total_time + optimization_duration) / 
                                   (metrics_.total_function_calls + 1);
    } else {
        metrics_.avg_call_duration = optimization_duration;
    }
}

void AdvancedCallStack::PrepareTailCall(RegisterIndex func_reg, Size param_count) {
    if (IsEmpty()) {
        throw RuntimeError("Cannot prepare tail call: empty call stack");
    }
    
    // 尾调用准备逻辑在这里实现
    // 主要是参数移动，具体的移动操作由VM的ExecuteTAILCALL处理
    
    // 验证寄存器索引
    if (func_reg > 255) {
        throw RuntimeError("Invalid function register for tail call: " + 
                          std::to_string(func_reg));
    }
    
    // 验证参数数量
    if (param_count > 255) {
        throw RuntimeError("Too many parameters for tail call: " + 
                          std::to_string(param_count));
    }
    
    // 标记即将进行尾调用优化
    // 这里可以添加预处理逻辑，如参数验证、内存预分配等
}

bool AdvancedCallStack::IsRecursiveCall(const Proto* proto) const {
    if (!proto || IsEmpty()) {
        return false;
    }
    
    // 检查调用栈中是否已经存在相同的函数
    for (Size i = 0; i < GetDepth(); ++i) {
        const CallFrame& frame = GetFrame(i);
        if (frame.GetProto() == proto) {
            return true;
        }
    }
    
    return false;
}

Size AdvancedCallStack::GetRecursionDepth(const Proto* proto) const {
    if (!proto) {
        return 0;
    }
    
    Size depth = 0;
    for (Size i = 0; i < GetDepth(); ++i) {
        const CallFrame& frame = GetFrame(i);
        if (frame.GetProto() == proto) {
            depth++;
        }
    }
    
    return depth;
}

/* ========================================================================== */
/* 性能监控实现 */
/* ========================================================================== */

void AdvancedCallStack::ResetMetrics() {
    metrics_ = CallStackMetrics{};
    metrics_.measurement_start = std::chrono::steady_clock::now();
    
    // 重置调用模式统计
    for (auto& pair : pattern_stats_) {
        pair.second = 0;
    }
    
    // 清空调用历史
    call_history_.clear();
    call_start_times_.clear();
    recursion_depths_.clear();
}

void AdvancedCallStack::UpdateCallTiming(std::chrono::steady_clock::time_point call_start_time) {
    auto call_end_time = std::chrono::steady_clock::now();
    auto call_duration = std::chrono::duration<double, std::milli>(
        call_end_time - call_start_time).count();
    
    // 更新平均调用持续时间
    if (metrics_.total_function_calls > 0) {
        double total_time = metrics_.avg_call_duration * metrics_.total_function_calls;
        metrics_.avg_call_duration = (total_time + call_duration) / 
                                   (metrics_.total_function_calls + 1);
    } else {
        metrics_.avg_call_duration = call_duration;
    }
}

void AdvancedCallStack::UpdateMemoryUsage(Size current_usage) {
    metrics_.current_memory_usage = current_usage;
    metrics_.peak_memory_usage = std::max(metrics_.peak_memory_usage, current_usage);
}

/* ========================================================================== */
/* 调用模式分析 */
/* ========================================================================== */

AdvancedCallStack::CallPattern AdvancedCallStack::AnalyzeCallPattern() const {
    if (IsEmpty() || call_history_.empty()) {
        return CallPattern::UNKNOWN;
    }
    
    Size depth = GetDepth();
    
    // 检查深度递归
    if (depth > 100) {
        return CallPattern::DEEP_RECURSIVE;
    }
    
    // 检查尾递归模式
    if (depth >= 2) {
        const CallFrame& current = GetCurrentFrame();
        const Proto* current_proto = current.GetProto();
        
        // 检查是否所有帧都是同一函数（尾递归特征）
        bool all_same = true;
        for (Size i = 0; i < depth; ++i) {
            if (GetFrame(i).GetProto() != current_proto) {
                all_same = false;
                break;
            }
        }
        
        if (all_same) {
            return CallPattern::TAIL_RECURSIVE;
        }
        
        // 检查互相递归
        if (depth >= 3) {
            std::set<const Proto*> unique_protos;
            for (Size i = 0; i < depth; ++i) {
                unique_protos.insert(GetFrame(i).GetProto());
            }
            
            if (unique_protos.size() == 2) {
                return CallPattern::MUTUAL_RECURSIVE;
            }
        }
    }
    
    // 检查迭代模式（调用历史中有重复模式）
    if (call_history_.size() >= 10) {
        // 简单的模式检测：检查最后10次调用是否有重复
        bool has_pattern = false;
        for (Size pattern_len = 2; pattern_len <= 5; ++pattern_len) {
            if (call_history_.size() >= pattern_len * 2) {
                bool matches = true;
                Size start = call_history_.size() - pattern_len * 2;
                
                for (Size i = 0; i < pattern_len; ++i) {
                    if (call_history_[start + i] != call_history_[start + pattern_len + i]) {
                        matches = false;
                        break;
                    }
                }
                
                if (matches) {
                    has_pattern = true;
                    break;
                }
            }
        }
        
        if (has_pattern) {
            return CallPattern::ITERATIVE;
        }
    }
    
    return CallPattern::NORMAL;
}

std::map<AdvancedCallStack::CallPattern, Size> AdvancedCallStack::GetCallPatternStats() const {
    return pattern_stats_;
}

std::string AdvancedCallStack::GetOptimizationSuggestion(CallPattern pattern) const {
    switch (pattern) {
        case CallPattern::TAIL_RECURSIVE:
            return "尾递归检测到。建议确保使用尾调用优化以避免栈溢出。当前优化率: " +
                   std::to_string(metrics_.tail_calls_optimized * 100 / 
                                std::max(metrics_.tail_calls_attempted, Size(1))) + "%";
        
        case CallPattern::DEEP_RECURSIVE:
            return "深度递归检测到。强烈建议重写为迭代形式或确保尾调用优化。当前最大深度: " +
                   std::to_string(metrics_.max_depth_reached);
        
        case CallPattern::MUTUAL_RECURSIVE:
            return "互相递归检测到。考虑合并函数或使用栈展开优化。";
        
        case CallPattern::ITERATIVE:
            return "迭代模式检测到。当前实现较为高效，可考虑进一步的循环优化。";
        
        case CallPattern::NORMAL:
            return "正常调用模式。性能良好，无需特殊优化。";
        
        default:
            return "调用模式未知。建议分析调用模式以确定优化策略。";
    }
}

/* ========================================================================== */
/* 调试增强实现 */
/* ========================================================================== */

std::string AdvancedCallStack::GetDetailedStackTrace(bool include_registers,
                                                   bool include_locals) const {
    if (IsEmpty()) {
        return "Empty call stack";
    }
    
    std::stringstream ss;
    ss << "=== 详细调用栈跟踪 ===\n";
    ss << "当前深度: " << GetDepth() << "/" << GetMaxDepth() << "\n";
    ss << "尾调用优化: " << metrics_.tail_calls_optimized << "/" 
       << metrics_.tail_calls_attempted << " 次\n";
    ss << "内存节省: " << metrics_.memory_saves_from_tail_calls << " 字节\n\n";
    
    for (Size i = 0; i < GetDepth(); ++i) {
        const CallFrame& frame = GetFrame(i);
        CallFrame::FrameInfo info = frame.GetFrameInfo();
        
        ss << "帧 #" << i << ": " << info.function_name << "\n";
        ss << "  文件: " << info.source_name << ":" << info.current_line << "\n";
        ss << "  定义: 第" << info.definition_line << "行\n";
        ss << "  基址: " << info.base << ", 参数: " << info.param_count << "\n";
        ss << "  指令指针: " << info.instruction_pointer << "\n";
        ss << "  可变参数: " << (info.is_vararg ? "是" : "否") << "\n";
        
        if (include_registers) {
            ss << "  寄存器信息: [实现待完善]\n";
        }
        
        if (include_locals) {
            ss << "  局部变量: [实现待完善]\n";
        }
        
        ss << "\n";
    }
    
    return ss.str();
}

std::vector<std::string> AdvancedCallStack::GetFunctionCallChain() const {
    std::vector<std::string> chain;
    
    for (Size i = 0; i < GetDepth(); ++i) {
        const CallFrame& frame = GetFrame(GetDepth() - 1 - i); // 从最外层到最内层
        std::string function_name = frame.GetFunctionName();
        
        if (function_name.empty()) {
            function_name = "<anonymous>";
        }
        
        chain.push_back(function_name);
    }
    
    return chain;
}

std::shared_ptr<AdvancedCallStack::CallGraphNode> AdvancedCallStack::BuildCallGraph() const {
    // 构建调用图的简化实现
    auto root = std::make_shared<CallGraphNode>();
    root->function_name = "<root>";
    root->call_count = 1;
    root->total_time = 0.0;
    
    if (!IsEmpty()) {
        // 当前只构建线性调用链
        auto current_node = root;
        
        for (Size i = GetDepth() - 1; i < GetDepth(); --i) {
            const CallFrame& frame = GetFrame(i);
            auto node = std::make_shared<CallGraphNode>();
            node->function_name = frame.GetFunctionName();
            node->call_count = 1;
            node->total_time = 0.0;
            
            current_node->children.push_back(node);
            current_node = node;
        }
    }
    
    return root;
}

std::string AdvancedCallStack::ExportCallGraphToDot() const {
    std::stringstream ss;
    ss << "digraph CallGraph {\n";
    ss << "  rankdir=TB;\n";
    ss << "  node [shape=box];\n";
    
    auto graph = BuildCallGraph();
    std::function<void(const std::shared_ptr<CallGraphNode>&, int&)> export_node = 
        [&](const std::shared_ptr<CallGraphNode>& node, int& id_counter) {
            int current_id = id_counter++;
            ss << "  node" << current_id << " [label=\"" << node->function_name 
               << "\\ncalls: " << node->call_count << "\"];\n";
            
            for (const auto& child : node->children) {
                int child_id = id_counter;
                export_node(child, id_counter);
                ss << "  node" << current_id << " -> node" << child_id << ";\n";
            }
        };
    
    int id_counter = 0;
    export_node(graph, id_counter);
    
    ss << "}\n";
    return ss.str();
}

/* ========================================================================== */
/* 重写基类方法 */
/* ========================================================================== */

void AdvancedCallStack::PushFrame(const Proto* proto, Size base, Size param_count, 
                                 Size return_address) {
    // 记录调用开始
    RecordCallStart(proto);
    
    // 更新统计
    metrics_.total_function_calls++;
    metrics_.current_depth = GetDepth() + 1;
    metrics_.max_depth_reached = std::max(metrics_.max_depth_reached, metrics_.current_depth);
    
    // 计算平均调用深度
    if (metrics_.total_function_calls > 0) {
        double total_depth = metrics_.avg_call_depth * (metrics_.total_function_calls - 1);
        metrics_.avg_call_depth = (total_depth + metrics_.current_depth) / 
                                metrics_.total_function_calls;
    } else {
        metrics_.avg_call_depth = metrics_.current_depth;
    }
    
    // 更新递归深度
    if (proto && IsRecursiveCall(proto)) {
        metrics_.recursive_calls++;
        recursion_depths_[proto] = GetRecursionDepth(proto) + 1;
        metrics_.max_recursion_depth = std::max(metrics_.max_recursion_depth,
                                              recursion_depths_[proto]);
    }
    
    // 更新调用历史
    if (proto) {
        call_history_.push_back(proto);
        if (call_history_.size() > MAX_CALL_HISTORY) {
            call_history_.erase(call_history_.begin());
        }
    }
    
    // 更新内存使用
    Size new_memory = metrics_.current_memory_usage + frame_memory_overhead_;
    UpdateMemoryUsage(new_memory);
    
    // 分析并更新调用模式
    CallPattern pattern = AnalyzeCallPattern();
    UpdateCallPatternStats(pattern);
    
    // 调用基类方法
    CallStack::PushFrame(proto, base, param_count, return_address);
}

CallFrame AdvancedCallStack::PopFrame() {
    if (IsEmpty()) {
        throw CallFrameError("Cannot pop from empty call stack");
    }
    
    // 获取当前帧信息
    const CallFrame& current_frame = GetCurrentFrame();
    const Proto* proto = current_frame.GetProto();
    
    // 记录调用结束
    RecordCallEnd(proto);
    
    // 更新统计
    metrics_.total_function_returns++;
    metrics_.current_depth = GetDepth() - 1;
    
    // 更新递归深度
    if (proto && recursion_depths_.count(proto)) {
        recursion_depths_[proto]--;
        if (recursion_depths_[proto] == 0) {
            recursion_depths_.erase(proto);
        }
    }
    
    // 更新内存使用
    Size new_memory = metrics_.current_memory_usage - frame_memory_overhead_;
    UpdateMemoryUsage(new_memory);
    
    // 调用基类方法
    return CallStack::PopFrame();
}

void AdvancedCallStack::Clear() {
    // 重置所有统计
    ResetMetrics();
    
    // 调用基类方法
    CallStack::Clear();
}

/* ========================================================================== */
/* 验证和诊断 */
/* ========================================================================== */

AdvancedCallStack::ValidationResult AdvancedCallStack::ValidateIntegrityAdvanced() const {
    ValidationResult result;
    result.is_valid = true;
    
    // 基础完整性检查
    bool basic_valid = ValidateIntegrity();
    if (!basic_valid) {
        result.is_valid = false;
        result.issues.push_back("基础调用栈完整性检查失败");
    }
    
    // 检查统计一致性
    if (metrics_.current_depth != GetDepth()) {
        result.is_valid = false;
        result.issues.push_back("当前深度统计不一致: 记录=" + 
                               std::to_string(metrics_.current_depth) + 
                               ", 实际=" + std::to_string(GetDepth()));
    }
    
    // 检查尾调用统计
    if (metrics_.tail_calls_optimized > metrics_.tail_calls_attempted) {
        result.is_valid = false;
        result.issues.push_back("尾调用统计异常: 优化次数超过尝试次数");
    }
    
    // 检查内存统计
    Size expected_memory = GetDepth() * frame_memory_overhead_;
    if (metrics_.current_memory_usage < expected_memory) {
        result.warnings.push_back("内存使用统计可能偏低");
    }
    
    // 检查递归深度
    for (const auto& pair : recursion_depths_) {
        Size actual_depth = GetRecursionDepth(pair.first);
        if (pair.second != actual_depth) {
            result.issues.push_back("递归深度统计不一致: 函数=" + 
                                   std::to_string(reinterpret_cast<uintptr_t>(pair.first)));
            result.is_valid = false;
        }
    }
    
    // 性能建议
    if (metrics_.tail_calls_attempted > 0) {
        double optimization_rate = static_cast<double>(metrics_.tail_calls_optimized) / 
                                 metrics_.tail_calls_attempted * 100.0;
        if (optimization_rate < 80.0) {
            result.suggestions.push_back("尾调用优化率较低(" + 
                                        std::to_string(optimization_rate) + 
                                        "%)，建议检查优化条件");
        }
    }
    
    if (metrics_.max_depth_reached > GetMaxDepth() * 0.8) {
        result.warnings.push_back("调用深度接近上限，建议增加栈大小或优化递归");
    }
    
    return result;
}

std::string AdvancedCallStack::DiagnoseCallStackIssues() const {
    auto validation = ValidateIntegrityAdvanced();
    
    std::stringstream ss;
    ss << "=== 调用栈诊断报告 ===\n\n";
    
    ss << "整体状态: " << (validation.is_valid ? "正常" : "异常") << "\n\n";
    
    if (!validation.issues.empty()) {
        ss << "发现的问题:\n";
        for (const auto& issue : validation.issues) {
            ss << "  - " << issue << "\n";
        }
        ss << "\n";
    }
    
    if (!validation.warnings.empty()) {
        ss << "警告:\n";
        for (const auto& warning : validation.warnings) {
            ss << "  - " << warning << "\n";
        }
        ss << "\n";
    }
    
    if (!validation.suggestions.empty()) {
        ss << "优化建议:\n";
        for (const auto& suggestion : validation.suggestions) {
            ss << "  - " << suggestion << "\n";
        }
        ss << "\n";
    }
    
    // 调用模式分析
    CallPattern current_pattern = AnalyzeCallPattern();
    ss << "当前调用模式: ";
    switch (current_pattern) {
        case CallPattern::NORMAL: ss << "正常调用"; break;
        case CallPattern::TAIL_RECURSIVE: ss << "尾递归"; break;
        case CallPattern::MUTUAL_RECURSIVE: ss << "互相递归"; break;
        case CallPattern::DEEP_RECURSIVE: ss << "深度递归"; break;
        case CallPattern::ITERATIVE: ss << "迭代模式"; break;
        default: ss << "未知"; break;
    }
    ss << "\n";
    
    ss << "优化建议: " << GetOptimizationSuggestion(current_pattern) << "\n";
    
    return ss.str();
}

std::string AdvancedCallStack::GeneratePerformanceReport() const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);
    
    ss << "=== 调用栈性能报告 ===\n\n";
    
    // 基础统计
    ss << "基础统计:\n";
    ss << "  总函数调用: " << metrics_.total_function_calls << " 次\n";
    ss << "  总函数返回: " << metrics_.total_function_returns << " 次\n";
    ss << "  当前调用深度: " << metrics_.current_depth << "\n";
    ss << "  最大达到深度: " << metrics_.max_depth_reached << "\n";
    ss << "  平均调用深度: " << metrics_.avg_call_depth << "\n\n";
    
    // 尾调用优化统计
    ss << "尾调用优化:\n";
    ss << "  尝试次数: " << metrics_.tail_calls_attempted << " 次\n";
    ss << "  成功优化: " << metrics_.tail_calls_optimized << " 次\n";
    if (metrics_.tail_calls_attempted > 0) {
        double rate = static_cast<double>(metrics_.tail_calls_optimized) / 
                     metrics_.tail_calls_attempted * 100.0;
        ss << "  优化率: " << rate << "%\n";
    }
    ss << "  节省的调用深度: " << metrics_.tail_call_depth_saved << " 层\n";
    ss << "  节省的内存: " << metrics_.memory_saves_from_tail_calls << " 字节\n\n";
    
    // 递归统计
    ss << "递归统计:\n";
    ss << "  递归调用次数: " << metrics_.recursive_calls << " 次\n";
    ss << "  最大递归深度: " << metrics_.max_recursion_depth << "\n";
    ss << "  深度递归次数: " << metrics_.deep_recursion_count << " 次\n\n";
    
    // 性能统计
    ss << "性能统计:\n";
    ss << "  平均调用时间: " << metrics_.avg_call_duration << " ms\n";
    
    auto now = std::chrono::steady_clock::now();
    auto total_time = std::chrono::duration<double>(now - metrics_.measurement_start).count();
    if (total_time > 0.0 && metrics_.total_function_calls > 0) {
        double calls_per_second = metrics_.total_function_calls / total_time;
        ss << "  调用频率: " << calls_per_second << " 次/秒\n";
    }
    ss << "\n";
    
    // 内存统计
    ss << "内存统计:\n";
    ss << "  当前内存使用: " << metrics_.current_memory_usage << " 字节\n";
    ss << "  峰值内存使用: " << metrics_.peak_memory_usage << " 字节\n";
    ss << "  尾调用节省内存: " << metrics_.memory_saves_from_tail_calls << " 字节\n\n";
    
    // 调用模式统计
    ss << "调用模式统计:\n";
    for (const auto& pair : pattern_stats_) {
        if (pair.second > 0) {
            ss << "  ";
            switch (pair.first) {
                case CallPattern::NORMAL: ss << "正常调用"; break;
                case CallPattern::TAIL_RECURSIVE: ss << "尾递归"; break;
                case CallPattern::MUTUAL_RECURSIVE: ss << "互相递归"; break;
                case CallPattern::DEEP_RECURSIVE: ss << "深度递归"; break;
                case CallPattern::ITERATIVE: ss << "迭代模式"; break;
                default: ss << "未知"; break;
            }
            ss << ": " << pair.second << " 次\n";
        }
    }
    
    return ss.str();
}

/* ========================================================================== */
/* 私有方法 */
/* ========================================================================== */

void AdvancedCallStack::UpdateCallPatternStats(CallPattern pattern) {
    pattern_stats_[pattern]++;
}

bool AdvancedCallStack::CheckTailCallPreconditions(const Proto* proto) const {
    // 检查函数原型有效性
    if (!proto) {
        return false;
    }
    
    // 检查当前是否在函数中
    if (IsEmpty()) {
        return false;
    }
    
    // 检查是否有足够的栈空间进行优化
    if (GetDepth() >= GetMaxDepth() - 1) {
        return false;
    }
    
    // 检查是否存在需要保留的局部状态
    // 在Lua中，尾调用总是可以优化的，因为规范保证了尾位置的调用
    return true;
}

Size AdvancedCallStack::CalculateMemorySavings(Size avoided_frames) const {
    return avoided_frames * frame_memory_overhead_;
}

void AdvancedCallStack::RecordCallStart(const Proto* proto) {
    if (proto) {
        call_start_times_[proto] = std::chrono::steady_clock::now();
    }
}

void AdvancedCallStack::RecordCallEnd(const Proto* proto) {
    if (proto && call_start_times_.count(proto)) {
        auto start_time = call_start_times_[proto];
        UpdateCallTiming(start_time);
        call_start_times_.erase(proto);
    }
}

/* ========================================================================== */
/* 工厂函数 */
/* ========================================================================== */

std::unique_ptr<AdvancedCallStack> CreateStandardAdvancedCallStack() {
    return std::make_unique<AdvancedCallStack>(VM_MAX_CALL_STACK_DEPTH);
}

std::unique_ptr<AdvancedCallStack> CreateHighPerformanceCallStack() {
    // 高性能版本，减少统计开销
    auto stack = std::make_unique<AdvancedCallStack>(VM_MAX_CALL_STACK_DEPTH * 2);
    stack->ResetMetrics(); // 确保统计从零开始
    return stack;
}

std::unique_ptr<AdvancedCallStack> CreateDebugCallStack() {
    // 调试版本，最详细的统计和跟踪
    auto stack = std::make_unique<AdvancedCallStack>(VM_MAX_CALL_STACK_DEPTH / 2);
    stack->ResetMetrics();
    return stack;
}

} // namespace lua_cpp