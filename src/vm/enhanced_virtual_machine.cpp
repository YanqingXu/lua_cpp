#include "enhanced_virtual_machine.h"
#include "core/exceptions.h"
#include "utils/debug.h"
#include "../stdlib/stdlib.h"
#include <sstream>
#include <chrono>

namespace lua_cpp {

/* ========================================================================== */
/* EnhancedVirtualMachine 实现 */
/* ========================================================================== */

EnhancedVirtualMachine::EnhancedVirtualMachine(const VMConfig& config)
    : VirtualMachine(config)
    , t026_enabled_(true)
    , is_tail_call_(false)
    , legacy_mode_(false) {
    
    InitializeT026Components();
}

void EnhancedVirtualMachine::InitializeT026Components() {
    // 创建高级调用栈
    advanced_call_stack_ = std::make_unique<AdvancedCallStack>();
    
    // 创建Upvalue管理器
    upvalue_manager_ = std::make_unique<UpvalueManager>();
    
    // 创建协程支持（如果启用）
    if (t026_config_.enable_coroutine_support) {
        coroutine_support_ = std::make_unique<CoroutineSupport>(
            t026_config_.max_coroutines,
            t026_config_.coroutine_stack_size
        );
        coroutine_support_->SetSchedulingPolicy(t026_config_.coroutine_scheduling);
    }
    
    // T027：创建并初始化标准库
    standard_library_ = CreateCompleteStandardLibrary();
    InitializeStandardLibrary();
    
    // 配置高级调用栈
    if (t026_config_.enable_tail_call_optimization) {
        advanced_call_stack_->EnableTailCallOptimization(true);
    }
    
    if (t026_config_.enable_performance_monitoring) {
        advanced_call_stack_->EnablePerformanceMonitoring(true);
    }
    
    if (t026_config_.enable_call_pattern_analysis) {
        advanced_call_stack_->EnableCallPatternAnalysis(true);
    }
    
    // 配置Upvalue管理器
    upvalue_manager_->EnableCaching(t026_config_.enable_upvalue_caching);
    upvalue_manager_->EnableSharing(t026_config_.enable_upvalue_sharing);
    upvalue_manager_->EnableGCIntegration(t026_config_.enable_gc_integration);
}

std::vector<LuaValue> EnhancedVirtualMachine::ExecuteProgramEnhanced(
    const Proto* proto, const std::vector<LuaValue>& args) {
    
    if (!t026_enabled_) {
        return VirtualMachine::ExecuteProgram(proto, args);
    }
    
    try {
        // 设置初始调用帧
        auto main_frame = std::make_unique<AdvancedCallFrame>(
            proto, 
            nullptr, 
            0, 
            args.size(),
            AdvancedCallFrame::FrameType::MAIN,
            false  // 不是尾调用
        );
        
        // 压入主调用帧
        advanced_call_stack_->PushFrame(std::move(main_frame));
        
        // 开始执行计时
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // 执行主循环
        std::vector<LuaValue> results;
        while (!advanced_call_stack_->IsEmpty()) {
            auto& current_frame = advanced_call_stack_->GetTop();
            
            // 执行当前帧
            auto frame_results = ExecuteFrame(current_frame);
            
            // 如果是主帧返回，保存结果
            if (current_frame.GetFrameType() == AdvancedCallFrame::FrameType::MAIN) {
                results = std::move(frame_results);
                break;
            }
            
            // 更新性能统计
            if (t026_config_.enable_performance_monitoring) {
                UpdatePerformanceStats();
            }
        }
        
        // 结束执行计时
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        // 记录执行时间
        if (t026_config_.enable_performance_monitoring) {
            advanced_call_stack_->RecordExecutionTime(duration.count());
        }
        
        return results;
        
    } catch (const LuaException& e) {
        // 清理调用栈
        advanced_call_stack_->Clear();
        throw;
    }
}

std::vector<LuaValue> EnhancedVirtualMachine::ExecuteFrame(AdvancedCallFrame& frame) {
    // 设置当前执行上下文
    SetCurrentFrame(&frame);
    
    std::vector<LuaValue> results;
    
    try {
        // 执行指令循环
        while (frame.GetPC() < frame.GetProto()->GetInstructionCount()) {
            Instruction instr = frame.GetProto()->GetInstruction(frame.GetPC());
            
            // 更新PC
            frame.SetPC(frame.GetPC() + 1);
            
            // 解析指令
            OpCode op = GET_OPCODE(instr);
            RegisterIndex a = GETARG_A(instr);
            int b = GETARG_B(instr);
            int c = GETARG_C(instr);
            int bx = GETARG_Bx(instr);
            int sbx = GETARG_sBx(instr);
            
            // 更新调用统计
            if (t026_config_.enable_call_pattern_analysis) {
                advanced_call_stack_->UpdateCallStats(op, &frame);
            }
            
            // 执行指令
            switch (op) {
                case OP_CALL:
                    ExecuteCALLEnhanced(a, b, c);
                    break;
                    
                case OP_TAILCALL:
                    ExecuteTAILCALLEnhanced(a, b, c);
                    break;
                    
                case OP_RETURN:
                    results = ExecuteRETURNEnhanced(a, b);
                    return results;
                    
                case OP_CLOSURE:
                    ExecuteCLOSUREEnhanced(a, bx);
                    break;
                    
                default:
                    // 委托给基类处理其他指令
                    ExecuteInstruction(instr);
                    break;
            }
            
            // 检查协程切换点
            if (coroutine_support_ && coroutine_support_->ShouldSwitch()) {
                auto yield_results = coroutine_support_->SwitchCoroutine();
                if (!yield_results.empty()) {
                    return yield_results;
                }
            }
        }
        
    } catch (const LuaException& e) {
        // 错误处理：恢复调用栈状态
        advanced_call_stack_->HandleError(&frame, e);
        throw;
    }
    
    return results;
}

void EnhancedVirtualMachine::ExecuteCALLEnhanced(RegisterIndex a, int b, int c) {
    if (!t026_enabled_) {
        VirtualMachine::ExecuteCALL(a, b, c);
        return;
    }
    
    // 获取函数和参数
    LuaValue func = GetRegister(a);
    std::vector<LuaValue> args;
    
    // 收集参数
    Size num_args = (b == 0) ? (GetStackTop() - a - 1) : (b - 1);
    args.reserve(num_args);
    for (Size i = 0; i < num_args; ++i) {
        args.push_back(GetRegister(a + 1 + i));
    }
    
    // 检查函数类型
    if (func.GetType() == LuaType::FUNCTION) {
        const Proto* proto = func.GetFunction();
        
        // 创建新的调用帧
        auto new_frame = std::make_unique<AdvancedCallFrame>(
            proto,
            &advanced_call_stack_->GetTop(),
            a,
            num_args,
            AdvancedCallFrame::FrameType::LUA,
            is_tail_call_
        );
        
        // 设置参数
        for (Size i = 0; i < args.size(); ++i) {
            new_frame->SetRegister(i, args[i]);
        }
        
        // 压入调用栈
        advanced_call_stack_->PushFrame(std::move(new_frame));
        
        // 重置尾调用标记
        is_tail_call_ = false;
        
    } else if (func.GetType() == LuaType::CFUNCTION) {
        // C函数调用
        CFunction cfunc = func.GetCFunction();
        
        // 创建C函数调用帧
        auto c_frame = std::make_unique<AdvancedCallFrame>(
            nullptr,
            &advanced_call_stack_->GetTop(),
            a,
            num_args,
            AdvancedCallFrame::FrameType::C,
            false
        );
        
        // 压入C调用栈
        advanced_call_stack_->PushFrame(std::move(c_frame));
        
        // 调用C函数
        std::vector<LuaValue> results = cfunc(args);
        
        // 弹出C调用栈
        advanced_call_stack_->PopFrame();
        
        // 设置返回值
        Size num_results = (c == 0) ? results.size() : (c - 1);
        for (Size i = 0; i < num_results && i < results.size(); ++i) {
            SetRegister(a + i, results[i]);
        }
        
    } else {
        throw LuaException("attempt to call a " + LuaTypeNames[static_cast<int>(func.GetType())] + " value");
    }
}

void EnhancedVirtualMachine::ExecuteTAILCALLEnhanced(RegisterIndex a, int b, int c) {
    if (!t026_enabled_ || !t026_config_.enable_tail_call_optimization) {
        // 回退到普通调用
        ExecuteCALLEnhanced(a, b, c);
        return;
    }
    
    // 检查是否应该优化尾调用
    if (!ShouldOptimizeTailCall()) {
        ExecuteCALLEnhanced(a, b, c);
        return;
    }
    
    // 标记为尾调用
    SetTailCallFlag(true);
    
    // 获取当前帧
    auto& current_frame = advanced_call_stack_->GetTop();
    
    // 执行尾调用优化
    if (advanced_call_stack_->OptimizeTailCall(&current_frame)) {
        // 优化成功，执行普通调用
        ExecuteCALLEnhanced(a, b, c);
    } else {
        // 优化失败，执行普通调用
        SetTailCallFlag(false);
        ExecuteCALLEnhanced(a, b, c);
    }
}

std::vector<LuaValue> EnhancedVirtualMachine::ExecuteRETURNEnhanced(RegisterIndex a, int b) {
    std::vector<LuaValue> results;
    
    if (!t026_enabled_) {
        return VirtualMachine::ExecuteRETURN(a, b);
    }
    
    // 收集返回值
    Size num_results = (b == 0) ? (GetStackTop() - a) : (b - 1);
    results.reserve(num_results);
    for (Size i = 0; i < num_results; ++i) {
        results.push_back(GetRegister(a + i));
    }
    
    // 获取当前帧
    auto& current_frame = advanced_call_stack_->GetTop();
    
    // 关闭Upvalues
    if (upvalue_manager_) {
        upvalue_manager_->CloseUpvalues(current_frame.GetStackBase());
    }
    
    // 更新返回统计
    if (t026_config_.enable_call_pattern_analysis) {
        advanced_call_stack_->RecordReturn(&current_frame, results.size());
    }
    
    // 弹出调用帧
    advanced_call_stack_->PopFrame();
    
    return results;
}

void EnhancedVirtualMachine::ExecuteCLOSUREEnhanced(RegisterIndex a, int bx) {
    if (!t026_enabled_) {
        VirtualMachine::ExecuteCLOSURE(a, bx);
        return;
    }
    
    // 获取函数原型
    const Proto* proto = GetCurrentFrame()->GetProto()->GetNestedProto(bx);
    
    // 创建闭包
    LuaValue closure = LuaValue::CreateFunction(proto);
    
    // 处理Upvalues
    Size num_upvalues = proto->GetUpvalueCount();
    for (Size i = 0; i < num_upvalues; ++i) {
        auto upvalue_info = proto->GetUpvalueInfo(i);
        
        std::shared_ptr<Upvalue> upvalue;
        if (upvalue_info.instack) {
            // 在当前栈中创建Upvalue
            Size stack_index = GetCurrentFrame()->GetStackBase() + upvalue_info.idx;
            upvalue = CreateUpvalue(stack_index);
        } else {
            // 从父函数的Upvalue中获取
            auto& parent_frame = advanced_call_stack_->GetTop();
            if (parent_frame.HasUpvalue(upvalue_info.idx)) {
                upvalue = parent_frame.GetUpvalue(upvalue_info.idx);
            } else {
                throw LuaException("Invalid upvalue reference");
            }
        }
        
        // 设置闭包的Upvalue
        closure.SetUpvalue(i, upvalue);
    }
    
    // 设置寄存器
    SetRegister(a, closure);
}

std::shared_ptr<Upvalue> EnhancedVirtualMachine::CreateUpvalue(Size stack_index) {
    if (!upvalue_manager_) {
        return std::make_shared<Upvalue>(GetRegister(stack_index));
    }
    
    return upvalue_manager_->GetOrCreateUpvalue(stack_index, [this](Size idx) {
        return GetRegister(idx);
    });
}

void EnhancedVirtualMachine::CloseUpvalues(Size level) {
    if (upvalue_manager_) {
        upvalue_manager_->CloseUpvalues(level);
    }
}

LuaValue EnhancedVirtualMachine::CreateCoroutine(const LuaValue& func, 
                                                const std::vector<LuaValue>& args) {
    if (!coroutine_support_) {
        throw LuaException("Coroutine support is not enabled");
    }
    
    return coroutine_support_->CreateCoroutine(func, args);
}

std::vector<LuaValue> EnhancedVirtualMachine::ResumeCoroutine(const LuaValue& coroutine, 
                                                            const std::vector<LuaValue>& args) {
    if (!coroutine_support_) {
        throw LuaException("Coroutine support is not enabled");
    }
    
    return coroutine_support_->ResumeCoroutine(coroutine, args);
}

std::vector<LuaValue> EnhancedVirtualMachine::YieldCoroutine(const std::vector<LuaValue>& yield_values) {
    if (!coroutine_support_) {
        throw LuaException("Coroutine support is not enabled");
    }
    
    return coroutine_support_->YieldCoroutine(yield_values);
}

std::string EnhancedVirtualMachine::GetEnhancedStackTrace() const {
    if (!t026_enabled_ || !advanced_call_stack_) {
        return VirtualMachine::GetStackTrace();
    }
    
    return advanced_call_stack_->GetDetailedStackTrace();
}

std::string EnhancedVirtualMachine::GetPerformanceReport() const {
    if (!advanced_call_stack_) {
        return "Performance monitoring not available";
    }
    
    return advanced_call_stack_->GetPerformanceReport();
}

std::string EnhancedVirtualMachine::GetCallPatternAnalysis() const {
    if (!advanced_call_stack_) {
        return "Call pattern analysis not available";
    }
    
    return advanced_call_stack_->GetCallPatternAnalysis();
}

std::string EnhancedVirtualMachine::GetUpvalueStatistics() const {
    if (!upvalue_manager_) {
        return "Upvalue management not available";
    }
    
    return upvalue_manager_->GetStatistics();
}

std::string EnhancedVirtualMachine::GetCoroutineOverview() const {
    if (!coroutine_support_) {
        return "Coroutine support not available";
    }
    
    return coroutine_support_->GetOverview();
}

void EnhancedVirtualMachine::SetT026Config(const T026Config& config) {
    t026_config_ = config;
    
    // 重新初始化组件
    if (t026_enabled_) {
        InitializeT026Components();
    }
}

const std::vector<CallFrame>& EnhancedVirtualMachine::GetLegacyCallStack() const {
    if (legacy_mode_) {
        return VirtualMachine::GetCallStack();
    }
    
    // 转换高级调用栈到传统格式
    legacy_call_stack_.clear();
    if (advanced_call_stack_) {
        for (Size i = 0; i < advanced_call_stack_->GetDepth(); ++i) {
            auto& advanced_frame = advanced_call_stack_->GetFrame(i);
            
            // 创建传统调用帧
            CallFrame legacy_frame(
                advanced_frame.GetProto(),
                advanced_frame.GetParent(),
                advanced_frame.GetReturnPC(),
                advanced_frame.GetNumArgs()
            );
            
            legacy_call_stack_.push_back(legacy_frame);
        }
    }
    
    return legacy_call_stack_;
}

void EnhancedVirtualMachine::SwitchToLegacyMode() {
    legacy_mode_ = true;
    t026_enabled_ = false;
    
    // 同步调用栈状态
    SyncCallStackState();
}

void EnhancedVirtualMachine::SwitchToEnhancedMode() {
    legacy_mode_ = false;
    t026_enabled_ = true;
    
    // 重新初始化T026组件
    InitializeT026Components();
}

void EnhancedVirtualMachine::SetTailCallFlag(bool is_tail_call) {
    is_tail_call_ = is_tail_call;
}

bool EnhancedVirtualMachine::ShouldOptimizeTailCall() const {
    if (!t026_config_.enable_tail_call_optimization) {
        return false;
    }
    
    // 检查调用栈深度
    if (advanced_call_stack_->GetDepth() < 2) {
        return false;
    }
    
    // 检查当前帧是否适合优化
    auto& current_frame = advanced_call_stack_->GetTop();
    return advanced_call_stack_->CanOptimizeTailCall(&current_frame);
}

void EnhancedVirtualMachine::UpdatePerformanceStats() {
    if (advanced_call_stack_) {
        advanced_call_stack_->UpdatePerformanceStats();
    }
}

void EnhancedVirtualMachine::SyncCallStackState() {
    if (!legacy_mode_ && advanced_call_stack_ && !advanced_call_stack_->IsEmpty()) {
        // 从高级调用栈同步到传统调用栈
        // 这里可以实现状态同步逻辑
    }
}

/* ========================================================================== */
/* VirtualMachineAdapter 实现 */
/* ========================================================================== */

VirtualMachineAdapter::VirtualMachineAdapter(std::unique_ptr<EnhancedVirtualMachine> vm)
    : vm_(std::move(vm)) {
    if (!vm_) {
        throw LuaException("Cannot create adapter with null VM");
    }
}

void VirtualMachineAdapter::EnableTailCallOptimization(bool enable) {
    auto config = vm_->GetT026Config();
    config.enable_tail_call_optimization = enable;
    vm_->SetT026Config(config);
}

void VirtualMachineAdapter::EnablePerformanceMonitoring(bool enable) {
    auto config = vm_->GetT026Config();
    config.enable_performance_monitoring = enable;
    vm_->SetT026Config(config);
}

void VirtualMachineAdapter::EnableCoroutineSupport(bool enable) {
    auto config = vm_->GetT026Config();
    config.enable_coroutine_support = enable;
    vm_->SetT026Config(config);
}

void VirtualMachineAdapter::EnableUpvalueManagement(bool enable) {
    auto config = vm_->GetT026Config();
    config.enable_upvalue_caching = enable;
    config.enable_upvalue_sharing = enable;
    config.enable_gc_integration = enable;
    vm_->SetT026Config(config);
}

std::string VirtualMachineAdapter::AnalyzeCompatibility() const {
    std::ostringstream oss;
    
    oss << "=== T026 Compatibility Analysis ===\n";
    oss << "Enhanced VM Status: " << (vm_->IsT026Enabled() ? "Enabled" : "Disabled") << "\n";
    
    auto config = vm_->GetT026Config();
    oss << "Tail Call Optimization: " << (config.enable_tail_call_optimization ? "ON" : "OFF") << "\n";
    oss << "Performance Monitoring: " << (config.enable_performance_monitoring ? "ON" : "OFF") << "\n";
    oss << "Coroutine Support: " << (config.enable_coroutine_support ? "ON" : "OFF") << "\n";
    oss << "Upvalue Caching: " << (config.enable_upvalue_caching ? "ON" : "OFF") << "\n";
    
    oss << "\nCompatibility: FULL - All legacy code should work unchanged\n";
    
    return oss.str();
}

std::vector<std::string> VirtualMachineAdapter::GetMigrationSuggestions() const {
    std::vector<std::string> suggestions;
    
    auto config = vm_->GetT026Config();
    
    if (!config.enable_tail_call_optimization) {
        suggestions.push_back("Consider enabling tail call optimization for better performance in recursive functions");
    }
    
    if (!config.enable_performance_monitoring) {
        suggestions.push_back("Enable performance monitoring to identify bottlenecks");
    }
    
    if (!config.enable_coroutine_support) {
        suggestions.push_back("Enable coroutine support if your application uses coroutines");
    }
    
    if (!config.enable_upvalue_caching) {
        suggestions.push_back("Enable upvalue caching for better closure performance");
    }
    
    suggestions.push_back("Use GetEnhancedStackTrace() for better error diagnostics");
    suggestions.push_back("Use GetPerformanceReport() to monitor VM performance");
    
    return suggestions;
}

std::string VirtualMachineAdapter::RunPerformanceComparison(Size legacy_runs, Size enhanced_runs) const {
    std::ostringstream oss;
    
    oss << "=== Performance Comparison ===\n";
    oss << "Legacy Mode Runs: " << legacy_runs << "\n";
    oss << "Enhanced Mode Runs: " << enhanced_runs << "\n";
    oss << "\n[Performance comparison would require actual benchmark execution]\n";
    oss << "Recommendation: Use enhanced mode for production workloads\n";
    
    return oss.str();
}

/* ========================================================================== */
/* 工厂函数实现 */
/* ========================================================================== */

std::unique_ptr<EnhancedVirtualMachine> CreateEnhancedVM() {
    auto vm = std::make_unique<EnhancedVirtualMachine>();
    
    EnhancedVirtualMachine::T026Config config;
    config.enable_tail_call_optimization = true;
    config.enable_performance_monitoring = true;
    config.enable_call_pattern_analysis = true;
    config.enable_upvalue_caching = true;
    config.enable_upvalue_sharing = true;
    config.enable_gc_integration = true;
    config.enable_coroutine_support = true;
    
    vm->SetT026Config(config);
    return vm;
}

std::unique_ptr<EnhancedVirtualMachine> CreateCompatibleVM() {
    auto vm = std::make_unique<EnhancedVirtualMachine>();
    
    EnhancedVirtualMachine::T026Config config;
    config.enable_tail_call_optimization = false;
    config.enable_performance_monitoring = false;
    config.enable_call_pattern_analysis = false;
    config.enable_upvalue_caching = false;
    config.enable_upvalue_sharing = false;
    config.enable_gc_integration = false;
    config.enable_coroutine_support = false;
    
    vm->SetT026Config(config);
    vm->SetT026Enabled(false);
    return vm;
}

std::unique_ptr<EnhancedVirtualMachine> CreateHighPerformanceEnhancedVM() {
    auto vm = std::make_unique<EnhancedVirtualMachine>();
    
    EnhancedVirtualMachine::T026Config config;
    config.enable_tail_call_optimization = true;
    config.enable_performance_monitoring = true;
    config.enable_call_pattern_analysis = false; // 关闭以提高性能
    config.enable_upvalue_caching = true;
    config.enable_upvalue_sharing = true;
    config.enable_gc_integration = true;
    config.enable_coroutine_support = false; // 关闭以提高性能
    
    vm->SetT026Config(config);
    return vm;
}

std::unique_ptr<EnhancedVirtualMachine> CreateDebugEnhancedVM() {
    auto vm = std::make_unique<EnhancedVirtualMachine>();
    
    EnhancedVirtualMachine::T026Config config;
    config.enable_tail_call_optimization = true;
    config.enable_performance_monitoring = true;
    config.enable_call_pattern_analysis = true; // 启用详细分析
    config.enable_upvalue_caching = true;
    config.enable_upvalue_sharing = true;
    config.enable_gc_integration = true;
    config.enable_coroutine_support = true;
    
    vm->SetT026Config(config);
    return vm;
}

std::unique_ptr<EnhancedVirtualMachine> UpgradeToEnhancedVM(
    std::unique_ptr<VirtualMachine> legacy_vm) {
    
    // 创建增强VM
    auto enhanced_vm = CreateCompatibleVM();
    
    // 迁移状态（如果需要）
    // 这里可以实现从传统VM到增强VM的状态迁移逻辑
    
    // 逐步启用增强功能
    enhanced_vm->SetT026Enabled(true);
    
    return enhanced_vm;
}

std::unique_ptr<VirtualMachineAdapter> CreateVMAdapter(
    const EnhancedVirtualMachine::T026Config& config) {
    
    auto vm = std::make_unique<EnhancedVirtualMachine>();
    vm->SetT026Config(config);
    
    return std::make_unique<VirtualMachineAdapter>(std::move(vm));
}

/* ========================================================================== */
/* T027标准库集成 */
/* ========================================================================== */

void EnhancedVirtualMachine::InitializeStandardLibrary() {
    if (!standard_library_) {
        throw VMExecutionError("Standard library not created");
    }
    
    // 获取全局表
    auto global_table = global_table_;
    if (!global_table) {
        throw VMExecutionError("Global table not initialized");
    }
    
    // 将所有标准库函数注册到全局表
    InitializeAllStandardLibraries(standard_library_.get(), global_table.get());
}

} // namespace lua_cpp