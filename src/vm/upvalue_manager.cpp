/**
 * @file upvalue_manager.cpp
 * @brief Upvalue管理器实现
 * @description 实现Lua闭包的Upvalue管理和生命周期控制
 * @author Lua C++ Project
 * @date 2025-09-26
 * @version T026 - Upvalue Management System
 */

#include "upvalue_manager.h"
#include "stack.h"
#include "../core/lua_common.h"
#include "../core/lua_errors.h"
#include "../memory/garbage_collector.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace lua_cpp {

/* ========================================================================== */
/* Upvalue类实现 */
/* ========================================================================== */

Upvalue::Upvalue(Size stack_index, LuaValue* stack_ptr)
    : is_closed_(false)
    , stack_index_(stack_index)
    , stack_value_ptr_(stack_ptr)
    , closed_value_()
    , ref_count_(0)
    , next_(nullptr)
    , prev_(nullptr) {
    
    if (!stack_ptr) {
        throw UpvalueError("Invalid stack pointer for upvalue");
    }
}

Upvalue::Upvalue(const LuaValue& closed_value)
    : is_closed_(true)
    , stack_index_(SIZE_MAX) // 无效索引标记
    , stack_value_ptr_(nullptr)
    , closed_value_(closed_value)
    , ref_count_(0)
    , next_(nullptr)
    , prev_(nullptr) {
}

LuaValue& Upvalue::GetValue() {
    if (is_closed_) {
        return closed_value_;
    } else {
        if (!stack_value_ptr_) {
            throw UpvalueAccessError("Invalid stack pointer for open upvalue");
        }
        return *stack_value_ptr_;
    }
}

const LuaValue& Upvalue::GetValue() const {
    if (is_closed_) {
        return closed_value_;
    } else {
        if (!stack_value_ptr_) {
            throw UpvalueAccessError("Invalid stack pointer for open upvalue");
        }
        return *stack_value_ptr_;
    }
}

void Upvalue::SetValue(const LuaValue& value) {
    if (is_closed_) {
        closed_value_ = value;
    } else {
        if (!stack_value_ptr_) {
            throw UpvalueAccessError("Invalid stack pointer for open upvalue");
        }
        *stack_value_ptr_ = value;
    }
}

void Upvalue::SetValue(LuaValue&& value) {
    if (is_closed_) {
        closed_value_ = std::move(value);
    } else {
        if (!stack_value_ptr_) {
            throw UpvalueAccessError("Invalid stack pointer for open upvalue");
        }
        *stack_value_ptr_ = std::move(value);
    }
}

LuaValue* Upvalue::GetValuePtr() {
    if (is_closed_) {
        return &closed_value_;
    } else {
        return stack_value_ptr_;
    }
}

const LuaValue* Upvalue::GetValuePtr() const {
    if (is_closed_) {
        return &closed_value_;
    } else {
        return stack_value_ptr_;
    }
}

void Upvalue::Close() {
    if (!is_closed_) {
        if (stack_value_ptr_) {
            // 将栈上的值复制到内部存储
            closed_value_ = *stack_value_ptr_;
        } else {
            // 如果栈指针无效，使用nil
            closed_value_ = LuaValue();
        }
        
        // 切换到闭合状态
        is_closed_ = true;
        stack_value_ptr_ = nullptr;
        stack_index_ = SIZE_MAX;
    }
}

bool Upvalue::PointsToStackIndex(Size stack_index) const {
    return !is_closed_ && stack_index_ == stack_index;
}

Upvalue::UpvalueInfo Upvalue::GetInfo() const {
    UpvalueInfo info;
    info.is_closed = is_closed_;
    info.stack_index = stack_index_;
    info.ref_count = ref_count_;
    
    const LuaValue& value = GetValue();
    info.value_type = value.TypeName();
    info.value_string = value.ToString();
    
    // 计算内存使用
    info.memory_usage = sizeof(Upvalue);
    if (is_closed_) {
        info.memory_usage += value.GetMemoryUsage();
    }
    
    return info;
}

std::string Upvalue::ToString() const {
    std::stringstream ss;
    ss << "Upvalue{";
    ss << "closed=" << (is_closed_ ? "true" : "false");
    if (!is_closed_) {
        ss << ", stack_index=" << stack_index_;
    }
    ss << ", ref_count=" << ref_count_;
    ss << ", value=" << GetValue().ToString();
    ss << "}";
    return ss.str();
}

bool Upvalue::ValidateIntegrity() const {
    // 检查基本状态一致性
    if (is_closed_) {
        if (stack_value_ptr_ != nullptr || stack_index_ != SIZE_MAX) {
            return false; // 闭合状态不应该有栈引用
        }
    } else {
        if (stack_value_ptr_ == nullptr || stack_index_ == SIZE_MAX) {
            return false; // 开放状态必须有有效栈引用
        }
    }
    
    // 检查引用计数
    if (ref_count_ < 0) {
        return false; // 引用计数不能为负
    }
    
    return true;
}

/* ========================================================================== */
/* UpvalueManager类实现 */
/* ========================================================================== */

UpvalueManager::UpvalueManager(LuaStack* stack)
    : stack_(stack)
    , upvalue_map_()
    , open_upvalue_head_(nullptr)
    , upvalue_cache_()
    , cache_access_counter_(0)
    , statistics_()
    , config_()
    , gc_() {
    
    if (!stack) {
        throw UpvalueError("Stack cannot be null for UpvalueManager");
    }
    
    // 初始化默认配置
    config_.enable_automatic_cleanup = true;
    config_.cleanup_threshold = 100;
    config_.enable_sharing_optimization = true;
    config_.enable_statistics = true;
    config_.max_upvalue_cache_size = 1000;
    
    ResetStatistics();
}

UpvalueManager::~UpvalueManager() {
    // 闭合所有开放的Upvalue
    CloseAllUpvalues();
    
    // 清理缓存
    upvalue_cache_.clear();
    
    // 清空映射表
    upvalue_map_.clear();
}

/* ========================================================================== */
/* 核心Upvalue操作 */
/* ========================================================================== */

std::shared_ptr<Upvalue> UpvalueManager::GetUpvalue(Size stack_index) {
    // 更新访问统计
    cache_access_counter_++;
    
    // 首先查找现有的Upvalue
    auto existing = FindUpvalue(stack_index);
    if (existing) {
        UpdateAccessStatistics(existing, true);
        return existing;
    }
    
    // 如果不存在，创建新的
    auto new_upvalue = CreateUpvalue(stack_index);
    UpdateAccessStatistics(new_upvalue, false);
    
    return new_upvalue;
}

std::shared_ptr<Upvalue> UpvalueManager::CreateUpvalue(Size stack_index) {
    // 验证栈索引
    if (stack_index >= stack_->GetCapacity()) {
        throw UpvalueError("Stack index out of bounds: " + std::to_string(stack_index));
    }
    
    // 获取栈值指针
    LuaValue* stack_ptr = &stack_->Get(stack_index);
    
    // 创建新的Upvalue
    auto upvalue = std::make_shared<Upvalue>(stack_index, stack_ptr);
    
    // 插入到映射表中
    upvalue_map_[stack_index] = upvalue;
    
    // 插入到有序链表中（用于快速遍历和闭合）
    InsertUpvalueOrdered(upvalue);
    
    // 更新统计
    if (config_.enable_statistics) {
        statistics_.upvalues_created++;
        statistics_.total_upvalues++;
        statistics_.open_upvalues++;
        statistics_.peak_upvalue_count = std::max(statistics_.peak_upvalue_count, 
                                                statistics_.total_upvalues);
    }
    
    // 检查是否需要自动清理
    if (config_.enable_automatic_cleanup && 
        upvalue_map_.size() >= config_.cleanup_threshold) {
        PerformAutomaticCleanup();
    }
    
    return upvalue;
}

std::shared_ptr<Upvalue> UpvalueManager::FindUpvalue(Size stack_index) {
    // 首先检查缓存
    auto cache_it = upvalue_cache_.find(stack_index);
    if (cache_it != upvalue_cache_.end()) {
        // 更新访问时间
        cache_it->second.last_access_time = cache_access_counter_;
        cache_it->second.access_count++;
        return cache_it->second.upvalue;
    }
    
    // 在主映射中查找
    auto it = upvalue_map_.find(stack_index);
    if (it != upvalue_map_.end()) {
        // 添加到缓存
        if (upvalue_cache_.size() < config_.max_upvalue_cache_size) {
            UpvalueCacheEntry entry;
            entry.upvalue = it->second;
            entry.last_access_time = cache_access_counter_;
            entry.access_count = 1;
            upvalue_cache_[stack_index] = entry;
        }
        
        return it->second;
    }
    
    return nullptr;
}

void UpvalueManager::CloseUpvalues(Size level) {
    std::vector<Size> to_close;
    
    // 收集需要闭合的Upvalue
    for (auto& pair : upvalue_map_) {
        Size stack_index = pair.first;
        auto& upvalue = pair.second;
        
        if (!upvalue->IsClosed() && stack_index >= level) {
            to_close.push_back(stack_index);
        }
    }
    
    // 闭合Upvalue
    for (Size stack_index : to_close) {
        auto it = upvalue_map_.find(stack_index);
        if (it != upvalue_map_.end()) {
            it->second->Close();
            
            // 更新统计
            if (config_.enable_statistics) {
                statistics_.upvalues_closed++;
                statistics_.open_upvalues--;
                statistics_.closed_upvalues++;
            }
            
            // 从缓存中移除
            upvalue_cache_.erase(stack_index);
        }
    }
}

void UpvalueManager::CloseAllUpvalues() {
    for (auto& pair : upvalue_map_) {
        auto& upvalue = pair.second;
        if (!upvalue->IsClosed()) {
            upvalue->Close();
            
            // 更新统计
            if (config_.enable_statistics) {
                statistics_.upvalues_closed++;
                statistics_.open_upvalues--;
                statistics_.closed_upvalues++;
            }
        }
    }
    
    // 清空缓存
    upvalue_cache_.clear();
}

void UpvalueManager::RemoveUpvalue(std::shared_ptr<Upvalue> upvalue) {
    if (!upvalue) {
        return;
    }
    
    // 从映射表中查找并移除
    for (auto it = upvalue_map_.begin(); it != upvalue_map_.end(); ++it) {
        if (it->second == upvalue) {
            Size stack_index = it->first;
            
            // 从链表中移除
            RemoveUpvalueFromList(upvalue);
            
            // 从映射表中移除
            upvalue_map_.erase(it);
            
            // 从缓存中移除
            upvalue_cache_.erase(stack_index);
            
            // 更新统计
            if (config_.enable_statistics) {
                statistics_.total_upvalues--;
                if (upvalue->IsClosed()) {
                    statistics_.closed_upvalues--;
                } else {
                    statistics_.open_upvalues--;
                }
            }
            
            break;
        }
    }
}

/* ========================================================================== */
/* 生命周期管理 */
/* ========================================================================== */

void UpvalueManager::AddReference(std::shared_ptr<Upvalue> upvalue) {
    if (upvalue) {
        upvalue->AddReference();
        
        // 更新统计
        if (config_.enable_statistics) {
            statistics_.total_references++;
        }
    }
}

bool UpvalueManager::RemoveReference(std::shared_ptr<Upvalue> upvalue) {
    if (!upvalue) {
        return false;
    }
    
    Size new_ref_count = upvalue->RemoveReference();
    
    // 更新统计
    if (config_.enable_statistics) {
        if (statistics_.total_references > 0) {
            statistics_.total_references--;
        }
    }
    
    // 如果引用计数为零，考虑回收
    if (new_ref_count == 0) {
        // 检查是否应该立即回收
        if (upvalue->IsClosed()) {
            // 闭合的Upvalue没有引用时可以安全回收
            RemoveUpvalue(upvalue);
            
            // 更新统计
            if (config_.enable_statistics) {
                statistics_.upvalues_collected++;
            }
            
            return true;
        }
    }
    
    return false;
}

Size UpvalueManager::CleanupUnreferencedUpvalues() {
    Size cleaned_count = 0;
    std::vector<std::shared_ptr<Upvalue>> to_remove;
    
    // 收集无引用的Upvalue
    for (auto& pair : upvalue_map_) {
        auto& upvalue = pair.second;
        if (!upvalue->HasReferences()) {
            to_remove.push_back(upvalue);
        }
    }
    
    // 移除无引用的Upvalue
    for (auto& upvalue : to_remove) {
        RemoveUpvalue(upvalue);
        cleaned_count++;
    }
    
    // 更新统计
    if (config_.enable_statistics) {
        statistics_.upvalues_collected += cleaned_count;
    }
    
    return cleaned_count;
}

Size UpvalueManager::ForceGarbageCollection() {
    Size collected_count = 0;
    
    // 首先清理无引用的Upvalue
    collected_count += CleanupUnreferencedUpvalues();
    
    // 清理缓存中的过期条目
    auto cache_it = upvalue_cache_.begin();
    while (cache_it != upvalue_cache_.end()) {
        // 检查缓存条目是否过期或无效
        if (!cache_it->second.upvalue || 
            cache_access_counter_ - cache_it->second.last_access_time > 1000) {
            cache_it = upvalue_cache_.erase(cache_it);
        } else {
            ++cache_it;
        }
    }
    
    return collected_count;
}

/* ========================================================================== */
/* 批量操作 */
/* ========================================================================== */

void UpvalueManager::UpdateStackReferences(LuaValue* old_stack, LuaValue* new_stack) {
    if (!old_stack || !new_stack) {
        return;
    }
    
    ptrdiff_t offset = new_stack - old_stack;
    
    // 更新所有开放Upvalue的栈指针
    for (auto& pair : upvalue_map_) {
        auto& upvalue = pair.second;
        if (!upvalue->IsClosed()) {
            LuaValue* old_ptr = upvalue->GetValuePtr();
            if (old_ptr >= old_stack && old_ptr < old_stack + stack_->GetCapacity()) {
                // 更新指针到新位置
                LuaValue* new_ptr = old_ptr + offset;
                // 这里需要访问Upvalue的私有成员，实际实现中可能需要友元函数
                // 或者提供专门的更新方法
            }
        }
    }
}

void UpvalueManager::MigrateUpvalue(Size old_index, Size new_index) {
    auto it = upvalue_map_.find(old_index);
    if (it != upvalue_map_.end()) {
        auto upvalue = it->second;
        
        // 更新映射表
        upvalue_map_.erase(it);
        upvalue_map_[new_index] = upvalue;
        
        // 更新缓存
        auto cache_it = upvalue_cache_.find(old_index);
        if (cache_it != upvalue_cache_.end()) {
            UpvalueCacheEntry entry = cache_it->second;
            upvalue_cache_.erase(cache_it);
            upvalue_cache_[new_index] = entry;
        }
        
        // 更新Upvalue内部的栈索引
        if (!upvalue->IsClosed()) {
            // 这里需要更新Upvalue内部状态
            // 实际实现中需要提供相应的接口
        }
    }
}

void UpvalueManager::Clear() {
    // 闭合所有Upvalue
    CloseAllUpvalues();
    
    // 清空所有容器
    upvalue_map_.clear();
    upvalue_cache_.clear();
    open_upvalue_head_.reset();
    
    // 重置统计
    ResetStatistics();
}

/* ========================================================================== */
/* 查询和统计 */
/* ========================================================================== */

Size UpvalueManager::GetOpenUpvalueCount() const {
    Size count = 0;
    for (const auto& pair : upvalue_map_) {
        if (!pair.second->IsClosed()) {
            count++;
        }
    }
    return count;
}

Size UpvalueManager::GetClosedUpvalueCount() const {
    Size count = 0;
    for (const auto& pair : upvalue_map_) {
        if (pair.second->IsClosed()) {
            count++;
        }
    }
    return count;
}

Size UpvalueManager::GetTotalReferenceCount() const {
    Size total = 0;
    for (const auto& pair : upvalue_map_) {
        total += pair.second->GetReferenceCount();
    }
    return total;
}

Size UpvalueManager::GetMemoryUsage() const {
    return CalculateMemoryUsage();
}

/* ========================================================================== */
/* 性能统计 */
/* ========================================================================== */

void UpvalueManager::ResetStatistics() {
    statistics_ = UpvalueStatistics{};
    cache_access_counter_ = 0;
}

void UpvalueManager::UpdateStatistics() {
    if (!config_.enable_statistics) {
        return;
    }
    
    statistics_.total_upvalues = upvalue_map_.size();
    statistics_.open_upvalues = GetOpenUpvalueCount();
    statistics_.closed_upvalues = GetClosedUpvalueCount();
    statistics_.total_references = GetTotalReferenceCount();
    statistics_.memory_usage = GetMemoryUsage();
    
    // 计算无引用的Upvalue数量
    statistics_.unreferenced_upvalues = 0;
    for (const auto& pair : upvalue_map_) {
        if (!pair.second->HasReferences()) {
            statistics_.unreferenced_upvalues++;
        }
    }
    
    // 计算共享的Upvalue数量（引用计数 > 1）
    statistics_.shared_upvalues = 0;
    for (const auto& pair : upvalue_map_) {
        if (pair.second->GetReferenceCount() > 1) {
            statistics_.shared_upvalues++;
        }
    }
    
    // 计算平均引用计数
    if (statistics_.total_upvalues > 0) {
        statistics_.avg_reference_count = static_cast<double>(statistics_.total_references) / 
                                        statistics_.total_upvalues;
    }
    
    // 计算命中率（基于缓存访问）
    if (cache_access_counter_ > 0) {
        Size cache_hits = 0;
        for (const auto& entry : upvalue_cache_) {
            cache_hits += entry.second.access_count;
        }
        statistics_.hit_rate = static_cast<double>(cache_hits) / cache_access_counter_;
    }
}

/* ========================================================================== */
/* 调试和诊断 */
/* ========================================================================== */

UpvalueManager::ValidationResult UpvalueManager::ValidateIntegrity() const {
    ValidationResult result;
    result.is_valid = true;
    
    // 验证每个Upvalue的完整性
    for (const auto& pair : upvalue_map_) {
        const auto& upvalue = pair.second;
        if (!upvalue->ValidateIntegrity()) {
            result.is_valid = false;
            result.issues.push_back("Upvalue integrity check failed for index: " + 
                                  std::to_string(pair.first));
        }
    }
    
    // 验证统计数据一致性
    Size actual_open = GetOpenUpvalueCount();
    Size actual_closed = GetClosedUpvalueCount();
    Size actual_total = upvalue_map_.size();
    
    if (config_.enable_statistics) {
        if (statistics_.open_upvalues != actual_open) {
            result.warnings.push_back("Open upvalue count mismatch: recorded=" + 
                                    std::to_string(statistics_.open_upvalues) + 
                                    ", actual=" + std::to_string(actual_open));
        }
        
        if (statistics_.closed_upvalues != actual_closed) {
            result.warnings.push_back("Closed upvalue count mismatch: recorded=" + 
                                    std::to_string(statistics_.closed_upvalues) + 
                                    ", actual=" + std::to_string(actual_closed));
        }
        
        if (statistics_.total_upvalues != actual_total) {
            result.warnings.push_back("Total upvalue count mismatch: recorded=" + 
                                    std::to_string(statistics_.total_upvalues) + 
                                    ", actual=" + std::to_string(actual_total));
        }
    }
    
    // 验证缓存一致性
    for (const auto& cache_entry : upvalue_cache_) {
        Size stack_index = cache_entry.first;
        auto map_it = upvalue_map_.find(stack_index);
        
        if (map_it == upvalue_map_.end() || 
            map_it->second != cache_entry.second.upvalue) {
            result.issues.push_back("Cache inconsistency for index: " + 
                                  std::to_string(stack_index));
            result.is_valid = false;
        }
    }
    
    // 性能建议
    if (statistics_.hit_rate < 0.8 && cache_access_counter_ > 100) {
        result.performance_tips.push_back("Low cache hit rate (" + 
                                        std::to_string(statistics_.hit_rate * 100) + 
                                        "%). Consider optimizing access patterns.");
    }
    
    if (statistics_.unreferenced_upvalues > statistics_.total_upvalues * 0.2) {
        result.performance_tips.push_back("High number of unreferenced upvalues (" + 
                                        std::to_string(statistics_.unreferenced_upvalues) + 
                                        "). Consider more frequent cleanup.");
    }
    
    return result;
}

std::string UpvalueManager::GetDebugInfo() const {
    std::stringstream ss;
    ss << "=== Upvalue Manager Debug Info ===\n";
    ss << "Total Upvalues: " << upvalue_map_.size() << "\n";
    ss << "Open Upvalues: " << GetOpenUpvalueCount() << "\n";
    ss << "Closed Upvalues: " << GetClosedUpvalueCount() << "\n";
    ss << "Cache Size: " << upvalue_cache_.size() << "\n";
    ss << "Total References: " << GetTotalReferenceCount() << "\n";
    ss << "Memory Usage: " << GetMemoryUsage() << " bytes\n";
    
    if (config_.enable_statistics) {
        ss << "\nStatistics:\n";
        ss << "  Created: " << statistics_.upvalues_created << "\n";
        ss << "  Closed: " << statistics_.upvalues_closed << "\n";
        ss << "  Collected: " << statistics_.upvalues_collected << "\n";
        ss << "  Shared: " << statistics_.shared_upvalues << "\n";
        ss << "  Hit Rate: " << std::fixed << std::setprecision(2) 
           << (statistics_.hit_rate * 100) << "%\n";
        ss << "  Avg Ref Count: " << std::fixed << std::setprecision(2) 
           << statistics_.avg_reference_count << "\n";
    }
    
    return ss.str();
}

std::string UpvalueManager::GenerateReport() const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);
    
    ss << "=== Upvalue Manager Report ===\n\n";
    
    // 基础信息
    ss << "Basic Information:\n";
    ss << "  Total Upvalues: " << upvalue_map_.size() << "\n";
    ss << "  Open Upvalues: " << GetOpenUpvalueCount() << "\n";
    ss << "  Closed Upvalues: " << GetClosedUpvalueCount() << "\n";
    ss << "  Total References: " << GetTotalReferenceCount() << "\n";
    ss << "  Memory Usage: " << GetMemoryUsage() << " bytes\n\n";
    
    // 缓存信息
    ss << "Cache Information:\n";
    ss << "  Cache Size: " << upvalue_cache_.size() << "/" 
       << config_.max_upvalue_cache_size << "\n";
    ss << "  Cache Accesses: " << cache_access_counter_ << "\n";
    if (config_.enable_statistics) {
        ss << "  Hit Rate: " << (statistics_.hit_rate * 100) << "%\n";
    }
    ss << "\n";
    
    // 统计信息
    if (config_.enable_statistics) {
        ss << "Statistics:\n";
        ss << "  Upvalues Created: " << statistics_.upvalues_created << "\n";
        ss << "  Upvalues Closed: " << statistics_.upvalues_closed << "\n";
        ss << "  Upvalues Collected: " << statistics_.upvalues_collected << "\n";
        ss << "  Shared Upvalues: " << statistics_.shared_upvalues << "\n";
        ss << "  Unreferenced: " << statistics_.unreferenced_upvalues << "\n";
        ss << "  Peak Count: " << statistics_.peak_upvalue_count << "\n";
        ss << "  Average References: " << statistics_.avg_reference_count << "\n\n";
    }
    
    // 配置信息
    ss << "Configuration:\n";
    ss << "  Auto Cleanup: " << (config_.enable_automatic_cleanup ? "Enabled" : "Disabled") << "\n";
    ss << "  Cleanup Threshold: " << config_.cleanup_threshold << "\n";
    ss << "  Sharing Optimization: " << (config_.enable_sharing_optimization ? "Enabled" : "Disabled") << "\n";
    ss << "  Statistics: " << (config_.enable_statistics ? "Enabled" : "Disabled") << "\n";
    ss << "  Max Cache Size: " << config_.max_upvalue_cache_size << "\n\n";
    
    // 验证结果
    auto validation = ValidateIntegrity();
    ss << "Integrity Check: " << (validation.is_valid ? "PASSED" : "FAILED") << "\n";
    
    if (!validation.issues.empty()) {
        ss << "Issues:\n";
        for (const auto& issue : validation.issues) {
            ss << "  - " << issue << "\n";
        }
    }
    
    if (!validation.warnings.empty()) {
        ss << "Warnings:\n";
        for (const auto& warning : validation.warnings) {
            ss << "  - " << warning << "\n";
        }
    }
    
    if (!validation.performance_tips.empty()) {
        ss << "Performance Tips:\n";
        for (const auto& tip : validation.performance_tips) {
            ss << "  - " << tip << "\n";
        }
    }
    
    return ss.str();
}

std::vector<Upvalue::UpvalueInfo> UpvalueManager::ExportUpvalueStates() const {
    std::vector<Upvalue::UpvalueInfo> states;
    states.reserve(upvalue_map_.size());
    
    for (const auto& pair : upvalue_map_) {
        states.push_back(pair.second->GetInfo());
    }
    
    return states;
}

bool UpvalueManager::CheckForMemoryLeaks() const {
    // 检查是否有无引用但未回收的Upvalue
    Size unreferenced_count = 0;
    for (const auto& pair : upvalue_map_) {
        if (!pair.second->HasReferences()) {
            unreferenced_count++;
        }
    }
    
    // 如果无引用的Upvalue超过20%，可能存在内存泄漏
    return unreferenced_count > upvalue_map_.size() * 0.2;
}

/* ========================================================================== */
/* 私有方法 */
/* ========================================================================== */

void UpvalueManager::InsertUpvalueOrdered(std::shared_ptr<Upvalue> upvalue) {
    // 简化实现：不维护有序链表
    // 实际应用中可以维护按栈索引排序的链表以优化闭合操作
}

void UpvalueManager::RemoveUpvalueFromList(std::shared_ptr<Upvalue> upvalue) {
    // 简化实现：与InsertUpvalueOrdered对应
}

void UpvalueManager::UpdateAccessStatistics(std::shared_ptr<Upvalue> upvalue, bool is_hit) {
    if (!config_.enable_statistics) {
        return;
    }
    
    // 更新命中率统计
    // 实际实现中可以维护更详细的统计信息
}

void UpvalueManager::PerformAutomaticCleanup() {
    if (!config_.enable_automatic_cleanup) {
        return;
    }
    
    // 执行清理操作
    CleanupUnreferencedUpvalues();
    
    // 清理缓存
    if (upvalue_cache_.size() > config_.max_upvalue_cache_size) {
        // 移除最少使用的缓存条目
        auto min_it = std::min_element(upvalue_cache_.begin(), upvalue_cache_.end(),
            [](const auto& a, const auto& b) {
                return a.second.last_access_time < b.second.last_access_time;
            });
        
        if (min_it != upvalue_cache_.end()) {
            upvalue_cache_.erase(min_it);
        }
    }
}

bool UpvalueManager::ValidateUpvalueList() const {
    // 验证链表结构完整性（如果维护链表的话）
    return true;
}

Size UpvalueManager::CalculateMemoryUsage() const {
    Size total = sizeof(UpvalueManager);
    
    // Upvalue存储
    total += upvalue_map_.size() * (sizeof(Size) + sizeof(std::shared_ptr<Upvalue>));
    
    for (const auto& pair : upvalue_map_) {
        total += sizeof(Upvalue);
        if (pair.second->IsClosed()) {
            total += pair.second->GetValue().GetMemoryUsage();
        }
    }
    
    // 缓存存储
    total += upvalue_cache_.size() * sizeof(UpvalueCacheEntry);
    
    return total;
}

/* ========================================================================== */
/* 工厂函数 */
/* ========================================================================== */

std::unique_ptr<UpvalueManager> CreateStandardUpvalueManager(LuaStack* stack) {
    return std::make_unique<UpvalueManager>(stack);
}

std::unique_ptr<UpvalueManager> CreateHighPerformanceUpvalueManager(LuaStack* stack) {
    auto manager = std::make_unique<UpvalueManager>(stack);
    
    // 高性能配置
    UpvalueManager::ManagerConfig config;
    config.enable_automatic_cleanup = true;
    config.cleanup_threshold = 500;  // 更高的阈值
    config.enable_sharing_optimization = true;
    config.enable_statistics = false;  // 禁用统计以提升性能
    config.max_upvalue_cache_size = 2000;  // 更大的缓存
    
    manager->SetConfig(config);
    return manager;
}

std::unique_ptr<UpvalueManager> CreateDebugUpvalueManager(LuaStack* stack) {
    auto manager = std::make_unique<UpvalueManager>(stack);
    
    // 调试配置
    UpvalueManager::ManagerConfig config;
    config.enable_automatic_cleanup = true;
    config.cleanup_threshold = 50;   // 更低的阈值，更频繁清理
    config.enable_sharing_optimization = true;
    config.enable_statistics = true; // 启用详细统计
    config.max_upvalue_cache_size = 100;  // 较小的缓存便于调试
    
    manager->SetConfig(config);
    return manager;
}

} // namespace lua_cpp