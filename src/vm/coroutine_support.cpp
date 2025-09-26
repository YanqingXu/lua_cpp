#include "coroutine_support.h"
#include "virtual_machine.h"
#include "core/proto.h"
#include <algorithm>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace lua_cpp {

/* ========================================================================== */
/* 协程状态工具函数 */
/* ========================================================================== */

std::string CoroutineStateToString(CoroutineState state) {
    switch (state) {
        case CoroutineState::SUSPENDED: return "suspended";
        case CoroutineState::RUNNING: return "running";
        case CoroutineState::NORMAL: return "normal";
        case CoroutineState::DEAD: return "dead";
        default: return "unknown";
    }
}

/* ========================================================================== */
/* CoroutineContext 实现 */
/* ========================================================================== */

CoroutineContext::CoroutineContext(Size initial_stack_size, Size max_call_depth)
    : state_(CoroutineState::SUSPENDED)
    , instruction_pointer_(0)
    , current_proto_(nullptr) {
    
    // 创建执行上下文组件
    call_stack_ = std::make_unique<AdvancedCallStack>(max_call_depth);
    lua_stack_ = std::make_unique<LuaStack>(initial_stack_size);
    upvalue_manager_ = std::make_unique<UpvalueManager>();
    
    // 初始化统计信息
    stats_.created_time = std::chrono::steady_clock::now();
    stats_.last_run_time = stats_.created_time;
}

/* ====================================================================== */
/* 上下文保存和恢复 */
/* ====================================================================== */

void CoroutineContext::SaveContextTo(CoroutineContext& target) const {
    // 保存状态
    target.state_ = state_;
    target.instruction_pointer_ = instruction_pointer_;
    target.current_proto_ = current_proto_;
    
    // 深拷贝栈状态（简化实现，实际需要根据具体Stack类接口）
    // 这里假设Stack有Clone方法
    // target.lua_stack_ = std::make_unique<LuaStack>(*lua_stack_);
    
    // 保存参数和返回值
    target.arguments_ = arguments_;
    target.return_values_ = return_values_;
    target.yield_values_ = yield_values_;
    
    // 保存统计信息
    target.stats_ = stats_;
}

void CoroutineContext::RestoreContextFrom(const CoroutineContext& source) {
    // 恢复状态
    state_ = source.state_;
    instruction_pointer_ = source.instruction_pointer_;
    current_proto_ = source.current_proto_;
    
    // 恢复参数和返回值
    arguments_ = source.arguments_;
    return_values_ = source.return_values_;
    yield_values_ = source.yield_values_;
    
    // 恢复统计信息
    stats_ = source.stats_;
}

void CoroutineContext::SwapContext(CoroutineContext& other) {
    // 交换状态
    std::swap(state_, other.state_);
    std::swap(instruction_pointer_, other.instruction_pointer_);
    std::swap(current_proto_, other.current_proto_);
    
    // 交换栈（需要特殊处理）
    call_stack_.swap(other.call_stack_);
    lua_stack_.swap(other.lua_stack_);
    upvalue_manager_.swap(other.upvalue_manager_);
    
    // 交换数据
    arguments_.swap(other.arguments_);
    return_values_.swap(other.return_values_);
    yield_values_.swap(other.yield_values_);
    
    // 交换统计信息
    std::swap(stats_, other.stats_);
}

/* ====================================================================== */
/* 协程参数和返回值 */
/* ====================================================================== */

void CoroutineContext::SetArguments(const std::vector<LuaValue>& args) {
    arguments_ = args;
    
    // 将参数压入Lua栈
    for (const auto& arg : args) {
        lua_stack_->Push(arg);
    }
}

void CoroutineContext::SetReturnValues(const std::vector<LuaValue>& values) {
    return_values_ = values;
}

void CoroutineContext::SetYieldValues(const std::vector<LuaValue>& values) {
    yield_values_ = values;
}

/* ====================================================================== */
/* 统计和诊断 */
/* ====================================================================== */

void CoroutineContext::ResetStats() {
    stats_ = CoroutineStats{};
    stats_.created_time = std::chrono::steady_clock::now();
    stats_.last_run_time = stats_.created_time;
}

void CoroutineContext::UpdateRunTimeStats(std::chrono::steady_clock::time_point run_start) {
    auto run_end = std::chrono::steady_clock::now();
    auto run_duration = std::chrono::duration<double>(run_end - run_start).count();
    
    stats_.total_run_time += run_duration;
    stats_.avg_run_time = stats_.total_run_time / std::max(Size{1}, stats_.resume_count);
    stats_.last_run_time = run_end;
    
    // 更新最大使用量统计
    if (lua_stack_) {
        stats_.max_stack_usage = std::max(stats_.max_stack_usage, lua_stack_->GetSize());
    }
    if (call_stack_) {
        stats_.max_call_depth = std::max(stats_.max_call_depth, call_stack_->GetDepth());
    }
}

Size CoroutineContext::GetMemoryUsage() const {
    Size usage = sizeof(*this);
    
    if (call_stack_) {
        usage += call_stack_->GetMemoryUsage();
    }
    if (lua_stack_) {
        usage += lua_stack_->GetMemoryUsage();
    }
    if (upvalue_manager_) {
        usage += upvalue_manager_->GetMemoryUsage();
    }
    
    // 添加向量的内存使用
    usage += arguments_.capacity() * sizeof(LuaValue);
    usage += return_values_.capacity() * sizeof(LuaValue);
    usage += yield_values_.capacity() * sizeof(LuaValue);
    
    return usage;
}

std::string CoroutineContext::GetDebugInfo() const {
    std::ostringstream oss;
    oss << "CoroutineContext Debug Info:\n";
    oss << "  State: " << CoroutineStateToString(state_) << "\n";
    oss << "  Instruction Pointer: " << instruction_pointer_ << "\n";
    oss << "  Current Proto: " << (current_proto_ ? "Valid" : "Null") << "\n";
    oss << "  Arguments: " << arguments_.size() << "\n";
    oss << "  Return Values: " << return_values_.size() << "\n";
    oss << "  Yield Values: " << yield_values_.size() << "\n";
    oss << "  Resume Count: " << stats_.resume_count << "\n";
    oss << "  Yield Count: " << stats_.yield_count << "\n";
    oss << "  Total Run Time: " << std::fixed << std::setprecision(6) << stats_.total_run_time << "s\n";
    oss << "  Average Run Time: " << std::fixed << std::setprecision(6) << stats_.avg_run_time << "s\n";
    oss << "  Max Stack Usage: " << stats_.max_stack_usage << "\n";
    oss << "  Max Call Depth: " << stats_.max_call_depth << "\n";
    oss << "  Memory Usage: " << GetMemoryUsage() << " bytes\n";
    
    return oss.str();
}

bool CoroutineContext::ValidateIntegrity() const {
    // 检查基本状态
    if (state_ < CoroutineState::SUSPENDED || state_ > CoroutineState::DEAD) {
        return false;
    }
    
    // 检查栈完整性
    if (!call_stack_ || !lua_stack_ || !upvalue_manager_) {
        return false;
    }
    
    if (!call_stack_->ValidateIntegrity() || 
        !upvalue_manager_->ValidateIntegrity()) {
        return false;
    }
    
    // 检查统计信息合理性
    if (stats_.resume_count < 0 || stats_.yield_count < 0) {
        return false;
    }
    
    return true;
}

/* ========================================================================== */
/* CoroutineScheduler 实现 */
/* ========================================================================== */

CoroutineScheduler::CoroutineScheduler()
    : next_coroutine_id_(1)
    , current_coroutine_id_(0)  // 0表示主线程
    , scheduling_policy_(SchedulingPolicy::COOPERATIVE) {
    
    // 创建主线程上下文
    main_thread_context_ = std::make_unique<CoroutineContext>();
    main_thread_context_->SetState(CoroutineState::RUNNING);
    
    // 初始化统计信息
    ResetStats();
}

/* ====================================================================== */
/* 协程生命周期管理 */
/* ====================================================================== */

CoroutineScheduler::CoroutineId CoroutineScheduler::CreateCoroutine(
    const Proto* proto, const std::vector<LuaValue>& args) {
    
    if (!proto) {
        throw CoroutineError("Cannot create coroutine with null proto");
    }
    
    auto id = GenerateCoroutineId();
    
    // 创建协程上下文
    auto context = std::make_shared<CoroutineContext>();
    context->SetState(CoroutineState::SUSPENDED);
    context->SetCurrentProto(proto);
    context->SetArguments(args);
    
    // 创建协程条目
    CoroutineEntry entry;
    entry.context = context;
    entry.priority = 0;  // 默认优先级
    entry.last_run_time = std::chrono::steady_clock::now();
    entry.total_run_count = 0;
    
    // 存储协程
    coroutines_[id] = std::move(entry);
    
    // 更新统计信息
    stats_.total_coroutines_created++;
    stats_.current_coroutine_count++;
    stats_.max_concurrent_coroutines = std::max(
        stats_.max_concurrent_coroutines, stats_.current_coroutine_count);
    
    return id;
}

void CoroutineScheduler::DestroyCoroutine(CoroutineId id) {
    auto it = coroutines_.find(id);
    if (it == coroutines_.end()) {
        return;  // 协程不存在，忽略
    }
    
    // 设置为死亡状态
    it->second.context->SetState(CoroutineState::DEAD);
    
    // 如果是当前运行的协程，切换回主线程
    if (current_coroutine_id_ == id) {
        SwitchToMainThread();
    }
    
    // 移除协程
    coroutines_.erase(it);
    
    // 更新统计信息
    stats_.total_coroutines_destroyed++;
    stats_.current_coroutine_count--;
}

std::shared_ptr<CoroutineContext> CoroutineScheduler::GetCoroutine(CoroutineId id) {
    if (id == 0) {
        return std::shared_ptr<CoroutineContext>(main_thread_context_.get(), [](CoroutineContext*){});
    }
    
    auto it = coroutines_.find(id);
    if (it == coroutines_.end()) {
        return nullptr;
    }
    
    return it->second.context;
}

std::shared_ptr<CoroutineContext> CoroutineScheduler::GetCurrentCoroutine() const {
    return const_cast<CoroutineScheduler*>(this)->GetCoroutine(current_coroutine_id_);
}

bool CoroutineScheduler::CoroutineExists(CoroutineId id) const {
    if (id == 0) {
        return true;  // 主线程总是存在
    }
    return coroutines_.find(id) != coroutines_.end();
}

/* ====================================================================== */
/* 协程调度 */
/* ====================================================================== */

std::vector<LuaValue> CoroutineScheduler::ResumeCoroutine(
    CoroutineId id, const std::vector<LuaValue>& args) {
    
    auto coroutine = GetCoroutine(id);
    if (!coroutine) {
        throw CoroutineError("Coroutine does not exist");
    }
    
    if (!coroutine->CanResume()) {
        throw CoroutineStateError("Coroutine cannot be resumed in current state: " + 
                                 CoroutineStateToString(coroutine->GetState()));
    }
    
    // 记录恢复开始时间
    auto resume_start = std::chrono::steady_clock::now();
    
    // 保存当前上下文
    auto old_current_id = current_coroutine_id_;
    
    try {
        // 切换到目标协程
        SwitchToCoroutine(id);
        
        // 设置resume参数
        coroutine->SetArguments(args);
        coroutine->SetState(CoroutineState::RUNNING);
        
        // 更新统计信息
        coroutine->stats_.resume_count++;
        stats_.total_resumes++;
        
        // 这里应该执行协程的字节码
        // 实际实现中需要调用VM的执行器
        // 暂时返回空值作为占位
        std::vector<LuaValue> result;
        
        // 模拟执行完成或yield
        if (coroutine->GetState() == CoroutineState::RUNNING) {
            // 如果还在运行状态，说明协程执行完毕
            coroutine->SetState(CoroutineState::DEAD);
            result = coroutine->GetReturnValues();
        } else {
            // 协程被yield了
            result = coroutine->GetYieldValues();
        }
        
        // 更新运行时统计
        coroutine->UpdateRunTimeStats(resume_start);
        
        return result;
        
    } catch (...) {
        // 恢复原来的上下文
        current_coroutine_id_ = old_current_id;
        if (auto current = GetCurrentCoroutine()) {
            current->SetState(CoroutineState::RUNNING);
        }
        throw;
    }
}

std::vector<LuaValue> CoroutineScheduler::YieldCoroutine(const std::vector<LuaValue>& yield_values) {
    auto coroutine = GetCurrentCoroutine();
    if (!coroutine) {
        throw CoroutineError("No current coroutine to yield");
    }
    
    if (!coroutine->CanYield()) {
        throw CoroutineStateError("Current coroutine cannot yield in state: " + 
                                 CoroutineStateToString(coroutine->GetState()));
    }
    
    // 设置yield值
    coroutine->SetYieldValues(yield_values);
    coroutine->SetState(CoroutineState::SUSPENDED);
    
    // 更新统计信息
    coroutine->stats_.yield_count++;
    stats_.total_yields++;
    
    // 切换回主线程或选择下一个协程
    if (scheduling_policy_ == SchedulingPolicy::COOPERATIVE) {
        SwitchToMainThread();
    } else {
        auto next_id = SelectNextCoroutine();
        if (next_id != current_coroutine_id_) {
            SwitchToCoroutine(next_id);
        }
    }
    
    // 返回resume时传入的参数
    return coroutine->GetArguments();
}

void CoroutineScheduler::SwitchToCoroutine(CoroutineId id) {
    if (current_coroutine_id_ == id) {
        return;  // 已经是当前协程
    }
    
    auto target = GetCoroutine(id);
    if (!target) {
        throw CoroutineError("Target coroutine does not exist");
    }
    
    // 执行上下文切换
    PerformContextSwitch(current_coroutine_id_, id);
    
    current_coroutine_id_ = id;
}

void CoroutineScheduler::SwitchToMainThread() {
    SwitchToCoroutine(0);
}

/* ====================================================================== */
/* 调度策略 */
/* ====================================================================== */

void CoroutineScheduler::SetCoroutinePriority(CoroutineId id, int priority) {
    auto it = coroutines_.find(id);
    if (it != coroutines_.end()) {
        it->second.priority = priority;
    }
}

int CoroutineScheduler::GetCoroutinePriority(CoroutineId id) const {
    auto it = coroutines_.find(id);
    if (it != coroutines_.end()) {
        return it->second.priority;
    }
    return 0;  // 默认优先级
}

/* ====================================================================== */
/* 批量操作 */
/* ====================================================================== */

std::vector<CoroutineScheduler::CoroutineId> CoroutineScheduler::GetAllCoroutineIds() const {
    std::vector<CoroutineId> ids;
    ids.reserve(coroutines_.size() + 1);
    
    ids.push_back(0);  // 主线程
    
    for (const auto& pair : coroutines_) {
        ids.push_back(pair.first);
    }
    
    return ids;
}

Size CoroutineScheduler::GetActiveCoroutineCount() const {
    Size count = 1;  // 主线程
    
    for (const auto& pair : coroutines_) {
        if (!pair.second.context->IsDead()) {
            count++;
        }
    }
    
    return count;
}

Size CoroutineScheduler::CleanupDeadCoroutines() {
    Size cleaned = 0;
    
    auto it = coroutines_.begin();
    while (it != coroutines_.end()) {
        if (it->second.context->IsDead()) {
            it = coroutines_.erase(it);
            cleaned++;
        } else {
            ++it;
        }
    }
    
    stats_.current_coroutine_count = coroutines_.size();
    
    return cleaned;
}

void CoroutineScheduler::SuspendAllCoroutines() {
    for (auto& pair : coroutines_) {
        if (pair.second.context->GetState() == CoroutineState::RUNNING ||
            pair.second.context->GetState() == CoroutineState::NORMAL) {
            pair.second.context->SetState(CoroutineState::SUSPENDED);
        }
    }
}

void CoroutineScheduler::DestroyAllCoroutines() {
    // 切换回主线程
    SwitchToMainThread();
    
    // 销毁所有协程
    for (auto& pair : coroutines_) {
        pair.second.context->SetState(CoroutineState::DEAD);
    }
    
    coroutines_.clear();
    
    // 更新统计信息
    stats_.current_coroutine_count = 0;
}

/* ====================================================================== */
/* 统计和监控 */
/* ====================================================================== */

void CoroutineScheduler::ResetStats() {
    stats_ = SchedulerStats{};
    stats_.current_coroutine_count = coroutines_.size();
}

void CoroutineScheduler::UpdateStats() {
    stats_.current_coroutine_count = coroutines_.size();
    
    // 计算内存使用量
    stats_.memory_usage = sizeof(*this);
    stats_.memory_usage += sizeof(*main_thread_context_);
    stats_.memory_usage += main_thread_context_->GetMemoryUsage();
    
    for (const auto& pair : coroutines_) {
        stats_.memory_usage += sizeof(CoroutineEntry);
        stats_.memory_usage += pair.second.context->GetMemoryUsage();
    }
}

std::string CoroutineScheduler::GetStatusReport() const {
    std::ostringstream oss;
    oss << "Coroutine Scheduler Status Report:\n";
    oss << "  Total Coroutines Created: " << stats_.total_coroutines_created << "\n";
    oss << "  Total Coroutines Destroyed: " << stats_.total_coroutines_destroyed << "\n";
    oss << "  Current Coroutine Count: " << stats_.current_coroutine_count << "\n";
    oss << "  Active Coroutine Count: " << GetActiveCoroutineCount() << "\n";
    oss << "  Current Running Coroutine: " << current_coroutine_id_ << "\n";
    oss << "  Total Context Switches: " << stats_.total_context_switches << "\n";
    oss << "  Total Resumes: " << stats_.total_resumes << "\n";
    oss << "  Total Yields: " << stats_.total_yields << "\n";
    oss << "  Average Switch Time: " << std::fixed << std::setprecision(3) 
        << stats_.avg_switch_time << " μs\n";
    oss << "  Max Concurrent Coroutines: " << stats_.max_concurrent_coroutines << "\n";
    oss << "  Memory Usage: " << stats_.memory_usage << " bytes\n";
    oss << "  Scheduling Policy: ";
    
    switch (scheduling_policy_) {
        case SchedulingPolicy::COOPERATIVE:
            oss << "Cooperative";
            break;
        case SchedulingPolicy::PREEMPTIVE:
            oss << "Preemptive";
            break;
        case SchedulingPolicy::PRIORITY:
            oss << "Priority";
            break;
    }
    oss << "\n";
    
    return oss.str();
}

/* ====================================================================== */
/* 调试和诊断 */
/* ====================================================================== */

std::string CoroutineScheduler::GetCoroutineOverview() const {
    std::ostringstream oss;
    oss << "Coroutine Overview:\n";
    
    // 主线程信息
    oss << "  Main Thread (ID: 0): " << CoroutineStateToString(main_thread_context_->GetState()) << "\n";
    
    // 所有协程信息
    for (const auto& pair : coroutines_) {
        const auto& entry = pair.second;
        oss << "  Coroutine " << pair.first << ": " 
            << CoroutineStateToString(entry.context->GetState())
            << " (Priority: " << entry.priority
            << ", Runs: " << entry.total_run_count << ")\n";
    }
    
    return oss.str();
}

std::string CoroutineScheduler::GetDebugInfo() const {
    std::ostringstream oss;
    oss << GetStatusReport() << "\n";
    oss << GetCoroutineOverview() << "\n";
    
    // 详细的协程信息
    oss << "Detailed Coroutine Information:\n";
    for (const auto& pair : coroutines_) {
        oss << "Coroutine " << pair.first << ":\n";
        oss << pair.second.context->GetDebugInfo() << "\n";
    }
    
    return oss.str();
}

bool CoroutineScheduler::ValidateIntegrity() const {
    // 检查主线程
    if (!main_thread_context_ || !main_thread_context_->ValidateIntegrity()) {
        return false;
    }
    
    // 检查当前协程ID的有效性
    if (!CoroutineExists(current_coroutine_id_)) {
        return false;
    }
    
    // 检查所有协程的完整性
    for (const auto& pair : coroutines_) {
        if (!pair.second.context || !pair.second.context->ValidateIntegrity()) {
            return false;
        }
    }
    
    // 检查统计信息的一致性
    if (stats_.current_coroutine_count != coroutines_.size()) {
        return false;
    }
    
    return true;
}

bool CoroutineScheduler::CheckForDeadlock() const {
    // 简单的死锁检测：检查是否所有协程都在等待
    Size suspended_count = 0;
    Size total_count = 0;
    
    for (const auto& pair : coroutines_) {
        if (!pair.second.context->IsDead()) {
            total_count++;
            if (pair.second.context->GetState() == CoroutineState::SUSPENDED) {
                suspended_count++;
            }
        }
    }
    
    // 如果所有活跃协程都被挂起，可能存在死锁
    return total_count > 0 && suspended_count == total_count;
}

/* ====================================================================== */
/* 内部方法 */
/* ====================================================================== */

CoroutineScheduler::CoroutineId CoroutineScheduler::GenerateCoroutineId() {
    return next_coroutine_id_++;
}

void CoroutineScheduler::PerformContextSwitch(CoroutineId from_id, CoroutineId to_id) {
    auto switch_start = std::chrono::steady_clock::now();
    
    auto from_context = GetCoroutine(from_id);
    auto to_context = GetCoroutine(to_id);
    
    if (!from_context || !to_context) {
        throw CoroutineError("Invalid context in switch operation");
    }
    
    // 保存当前协程状态
    if (from_context->GetState() == CoroutineState::RUNNING) {
        from_context->SetState(CoroutineState::NORMAL);
    }
    
    // 激活目标协程
    to_context->SetState(CoroutineState::RUNNING);
    
    // 实际的上下文切换逻辑在这里
    // 这里应该涉及CPU寄存器、栈指针等的保存和恢复
    // 简化实现中我们跳过这些底层细节
    
    // 更新统计信息
    stats_.total_context_switches++;
    UpdateSwitchTimeStats(switch_start);
}

CoroutineScheduler::CoroutineId CoroutineScheduler::SelectNextCoroutine() const {
    if (coroutines_.empty()) {
        return 0;  // 返回主线程
    }
    
    switch (scheduling_policy_) {
        case SchedulingPolicy::COOPERATIVE:
            return 0;  // 协作式调度总是返回主线程
            
        case SchedulingPolicy::PREEMPTIVE: {
            // 简单的轮转调度
            auto it = coroutines_.upper_bound(current_coroutine_id_);
            if (it == coroutines_.end()) {
                it = coroutines_.begin();
            }
            return it->first;
        }
        
        case SchedulingPolicy::PRIORITY: {
            // 优先级调度：选择优先级最高的可运行协程
            CoroutineId best_id = 0;
            int best_priority = INT_MAX;
            
            for (const auto& pair : coroutines_) {
                if (pair.second.context->CanResume() && 
                    pair.second.priority < best_priority) {
                    best_priority = pair.second.priority;
                    best_id = pair.first;
                }
            }
            
            return best_id;
        }
        
        default:
            return 0;
    }
}

void CoroutineScheduler::UpdateSwitchTimeStats(std::chrono::steady_clock::time_point switch_start) {
    auto switch_end = std::chrono::steady_clock::now();
    auto switch_time_us = std::chrono::duration<double, std::micro>(switch_end - switch_start).count();
    
    // 更新平均切换时间
    if (stats_.total_context_switches > 0) {
        stats_.avg_switch_time = (stats_.avg_switch_time * (stats_.total_context_switches - 1) + switch_time_us) 
                               / stats_.total_context_switches;
    } else {
        stats_.avg_switch_time = switch_time_us;
    }
}

/* ========================================================================== */
/* CoroutineSupport 实现 */
/* ========================================================================== */

CoroutineSupport::CoroutineSupport(VirtualMachine* vm)
    : vm_(vm)
    , next_coroutine_handle_(1) {
    
    if (!vm_) {
        throw CoroutineError("VirtualMachine pointer cannot be null");
    }
    
    // 设置默认配置
    config_ = CoroutineConfig{};
}

/* ====================================================================== */
/* 协程操作接口 */
/* ====================================================================== */

LuaValue CoroutineSupport::CreateCoroutine(const LuaValue& func, const std::vector<LuaValue>& args) {
    // 验证函数参数
    if (func.GetType() != LuaValueType::Function) {
        throw CoroutineError("Coroutine function must be a function value");
    }
    
    // 获取函数原型（这里需要根据实际的LuaValue实现）
    // 假设LuaValue有GetProto方法
    // const Proto* proto = func.GetProto();
    const Proto* proto = nullptr;  // 占位实现
    
    if (!proto) {
        throw CoroutineError("Cannot extract proto from function value");
    }
    
    // 创建协程
    auto coroutine_id = scheduler_.CreateCoroutine(proto, args);
    
    // 创建协程句柄
    auto handle = next_coroutine_handle_++;
    coroutine_map_[handle] = coroutine_id;
    
    // 返回协程对象（这里需要创建特殊的LuaValue）
    return CoroutineIdToLuaValue(handle);
}

std::vector<LuaValue> CoroutineSupport::Resume(const LuaValue& coroutine, const std::vector<LuaValue>& args) {
    if (!IsValidCoroutine(coroutine)) {
        throw CoroutineError("Invalid coroutine object");
    }
    
    auto coroutine_id = LuaValueToCoroutineId(coroutine);
    return scheduler_.ResumeCoroutine(coroutine_id, args);
}

std::vector<LuaValue> CoroutineSupport::Yield(const std::vector<LuaValue>& yield_values) {
    return scheduler_.YieldCoroutine(yield_values);
}

std::string CoroutineSupport::GetCoroutineStatus(const LuaValue& coroutine) {
    if (!IsValidCoroutine(coroutine)) {
        return "invalid";
    }
    
    auto coroutine_id = LuaValueToCoroutineId(coroutine);
    auto context = scheduler_.GetCoroutine(coroutine_id);
    
    if (!context) {
        return "dead";
    }
    
    return CoroutineStateToString(context->GetState());
}

bool CoroutineSupport::IsInCoroutine() const {
    return scheduler_.GetCurrentCoroutineId() != 0;
}

LuaValue CoroutineSupport::GetRunningCoroutine() const {
    auto current_id = scheduler_.GetCurrentCoroutineId();
    if (current_id == 0) {
        return LuaValue::Nil();  // 主线程返回nil
    }
    
    // 查找对应的句柄
    for (const auto& pair : coroutine_map_) {
        if (pair.second == current_id) {
            return CoroutineIdToLuaValue(pair.first);
        }
    }
    
    return LuaValue::Nil();
}

/* ====================================================================== */
/* 调度器访问 */
/* ====================================================================== */

void CoroutineSupport::SetSchedulingPolicy(CoroutineScheduler::SchedulingPolicy policy) {
    scheduler_.SetSchedulingPolicy(policy);
}

void CoroutineSupport::Cleanup() {
    scheduler_.DestroyAllCoroutines();
    coroutine_map_.clear();
    next_coroutine_handle_ = 1;
}

/* ====================================================================== */
/* 配置和优化 */
/* ====================================================================== */

std::string CoroutineSupport::GetStatisticsReport() const {
    std::ostringstream oss;
    oss << "Coroutine Support Statistics:\n";
    oss << scheduler_.GetStatusReport() << "\n";
    
    oss << "Configuration:\n";
    oss << "  Max Coroutines: " << config_.max_coroutines << "\n";
    oss << "  Default Stack Size: " << config_.default_stack_size << "\n";
    oss << "  Default Call Depth: " << config_.default_call_depth << "\n";
    oss << "  Enable Preemption: " << (config_.enable_preemption ? "Yes" : "No") << "\n";
    oss << "  Time Slice: " << config_.time_slice_ms << " ms\n";
    oss << "  Enable Priority Scheduling: " << (config_.enable_priority_scheduling ? "Yes" : "No") << "\n";
    oss << "  Enable Statistics: " << (config_.enable_statistics ? "Yes" : "No") << "\n";
    oss << "  Enable GC Integration: " << (config_.enable_gc_integration ? "Yes" : "No") << "\n";
    
    return oss.str();
}

/* ====================================================================== */
/* 内部方法 */
/* ====================================================================== */

LuaValue CoroutineSupport::CoroutineIdToLuaValue(CoroutineScheduler::CoroutineId id) const {
    // 这里需要创建特殊的协程LuaValue
    // 简化实现中返回数字类型
    return LuaValue::Number(static_cast<double>(id));
}

CoroutineScheduler::CoroutineId CoroutineSupport::LuaValueToCoroutineId(const LuaValue& value) const {
    // 从LuaValue中提取协程ID
    if (value.GetType() == LuaValueType::Number) {
        auto handle = static_cast<Size>(value.GetNumber());
        auto it = coroutine_map_.find(handle);
        if (it != coroutine_map_.end()) {
            return it->second;
        }
    }
    
    return 0;  // 无效ID
}

bool CoroutineSupport::IsValidCoroutine(const LuaValue& value) const {
    return LuaValueToCoroutineId(value) != 0;
}

/* ========================================================================== */
/* 工厂函数 */
/* ========================================================================== */

std::unique_ptr<CoroutineSupport> CreateStandardCoroutineSupport(VirtualMachine* vm) {
    auto support = std::make_unique<CoroutineSupport>(vm);
    
    CoroutineSupport::CoroutineConfig config;
    config.max_coroutines = 100;
    config.default_stack_size = 256;
    config.default_call_depth = 100;
    config.enable_preemption = false;
    config.enable_priority_scheduling = false;
    config.enable_statistics = true;
    config.enable_gc_integration = true;
    
    support->SetConfig(config);
    return support;
}

std::unique_ptr<CoroutineSupport> CreateHighPerformanceCoroutineSupport(VirtualMachine* vm) {
    auto support = std::make_unique<CoroutineSupport>(vm);
    
    CoroutineSupport::CoroutineConfig config;
    config.max_coroutines = 1000;
    config.default_stack_size = 512;
    config.default_call_depth = 200;
    config.enable_preemption = true;
    config.time_slice_ms = 5;
    config.enable_priority_scheduling = true;
    config.enable_statistics = false;  // 关闭统计以提高性能
    config.enable_gc_integration = true;
    
    support->SetConfig(config);
    return support;
}

std::unique_ptr<CoroutineSupport> CreateDebugCoroutineSupport(VirtualMachine* vm) {
    auto support = std::make_unique<CoroutineSupport>(vm);
    
    CoroutineSupport::CoroutineConfig config;
    config.max_coroutines = 50;
    config.default_stack_size = 128;
    config.default_call_depth = 50;
    config.enable_preemption = false;
    config.enable_priority_scheduling = false;
    config.enable_statistics = true;
    config.enable_gc_integration = true;
    
    support->SetConfig(config);
    return support;
}

} // namespace lua_cpp