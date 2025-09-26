/**
 * @file register_allocator.cpp
 * @brief 寄存器分配器实现
 * @description 管理Lua虚拟机寄存器的分配和释放
 * @author Lua C++ Project
 * @date 2025-09-25
 */

#include "register_allocator.h"
#include "../core/lua_errors.h"
#include <algorithm>
#include <sstream>

namespace lua_cpp {

/* ========================================================================== */
/* RegisterAllocator 实现 */
/* ========================================================================== */

RegisterAllocator::RegisterAllocator(Size max_registers)
    : max_registers_(max_registers)
    , next_register_(0)
    , register_top_(0)
    , temp_top_(0) {
    
    // 预分配寄存器标记数组
    free_registers_.resize(max_registers_, true);
    register_info_.reserve(max_registers_);
}

RegisterAllocator::~RegisterAllocator() = default;

RegisterIndex RegisterAllocator::Allocate() {
    // 查找第一个可用的寄存器
    for (RegisterIndex i = 0; i < max_registers_; ++i) {
        if (free_registers_[i]) {
            free_registers_[i] = false;
            register_top_ = std::max(register_top_, static_cast<Size>(i + 1));
            
            // 记录寄存器信息
            if (i >= register_info_.size()) {
                register_info_.resize(i + 1);
            }
            register_info_[i] = RegisterInfo{RegisterType::Local, "", i, false};
            
            return i;
        }
    }
    
    throw CompilerError("Register allocation failed: no free registers available");
}

RegisterIndex RegisterAllocator::AllocateNamed(const std::string& name) {
    RegisterIndex reg = Allocate();
    
    // 设置寄存器名称
    register_info_[reg].name = name;
    
    return reg;
}

RegisterIndex RegisterAllocator::AllocateTemporary() {
    RegisterIndex reg = Allocate();
    
    // 标记为临时寄存器
    register_info_[reg].type = RegisterType::Temporary;
    register_info_[reg].name = "temp_" + std::to_string(reg);
    register_info_[reg].is_temp = true;
    
    // 记录临时寄存器栈顶
    temp_top_ = std::max(temp_top_, register_top_);
    
    return reg;
}

RegisterIndex RegisterAllocator::AllocateRange(Size count) {
    if (count == 0) {
        throw CompilerError("Cannot allocate zero registers");
    }
    
    // 查找连续的可用寄存器
    for (RegisterIndex start = 0; start <= max_registers_ - count; ++start) {
        bool available = true;
        
        // 检查连续性
        for (Size i = 0; i < count; ++i) {
            if (!free_registers_[start + i]) {
                available = false;
                break;
            }
        }
        
        if (available) {
            // 分配整个范围
            for (Size i = 0; i < count; ++i) {
                RegisterIndex reg = start + static_cast<RegisterIndex>(i);
                free_registers_[reg] = false;
                
                // 设置寄存器信息
                if (reg >= register_info_.size()) {
                    register_info_.resize(reg + 1);
                }
                register_info_[reg] = RegisterInfo{
                    RegisterType::Local,
                    "range_" + std::to_string(start) + "_" + std::to_string(i),
                    reg,
                    false
                };
            }
            
            register_top_ = std::max(register_top_, static_cast<Size>(start + count));
            return start;
        }
    }
    
    throw CompilerError("Register allocation failed: cannot allocate " + 
                       std::to_string(count) + " consecutive registers");
}

void RegisterAllocator::Free(RegisterIndex reg) {
    if (reg >= max_registers_) {
        throw CompilerError("Invalid register index for free: " + std::to_string(reg));
    }
    
    if (free_registers_[reg]) {
        // 重复释放是允许的，但应该记录警告
        return;
    }
    
    free_registers_[reg] = true;
    
    // 清除寄存器信息
    if (reg < register_info_.size()) {
        register_info_[reg] = RegisterInfo{};
    }
    
    // 如果是最高寄存器，尝试收缩栈顶
    if (reg == register_top_ - 1) {
        while (register_top_ > 0 && free_registers_[register_top_ - 1]) {
            --register_top_;
        }
    }
}

void RegisterAllocator::FreeRange(RegisterIndex start, Size count) {
    for (Size i = 0; i < count; ++i) {
        Free(start + static_cast<RegisterIndex>(i));
    }
}

void RegisterAllocator::FreeTemporaries(Size saved_top) {
    // 释放所有临时寄存器到指定的栈顶
    for (RegisterIndex reg = static_cast<RegisterIndex>(saved_top); reg < register_top_; ++reg) {
        if (reg < register_info_.size() && register_info_[reg].is_temp) {
            Free(reg);
        }
    }
    
    register_top_ = saved_top;
    temp_top_ = saved_top;
}

void RegisterAllocator::FreeAllTemporaries() {
    // 释放所有临时寄存器
    for (RegisterIndex reg = 0; reg < register_top_; ++reg) {
        if (reg < register_info_.size() && register_info_[reg].is_temp) {
            Free(reg);
        }
    }
    
    // 重新计算栈顶
    RecalculateTop();
}

Size RegisterAllocator::GetTop() const {
    return register_top_;
}

void RegisterAllocator::SetTop(Size top) {
    if (top > max_registers_) {
        throw CompilerError("Register top exceeds maximum: " + std::to_string(top));
    }
    
    // 如果减少栈顶，释放高位寄存器
    if (top < register_top_) {
        for (RegisterIndex reg = static_cast<RegisterIndex>(top); reg < register_top_; ++reg) {
            Free(reg);
        }
    }
    
    register_top_ = top;
}

Size RegisterAllocator::GetTempTop() const {
    return temp_top_;
}

Size RegisterAllocator::SaveTempTop() {
    Size saved = temp_top_;
    temp_markers_.push(saved);
    return saved;
}

void RegisterAllocator::RestoreTempTop() {
    if (!temp_markers_.empty()) {
        Size saved_top = temp_markers_.top();
        temp_markers_.pop();
        FreeTemporaries(saved_top);
    }
}

Size RegisterAllocator::GetFreeCount() const {
    Size count = 0;
    for (Size i = 0; i < max_registers_; ++i) {
        if (free_registers_[i]) {
            ++count;
        }
    }
    return count;
}

Size RegisterAllocator::GetUsedCount() const {
    return max_registers_ - GetFreeCount();
}

bool RegisterAllocator::IsAllocated(RegisterIndex reg) const {
    return reg < max_registers_ && !free_registers_[reg];
}

bool RegisterAllocator::IsFree(RegisterIndex reg) const {
    return reg < max_registers_ && free_registers_[reg];
}

bool RegisterAllocator::IsTemporary(RegisterIndex reg) const {
    return reg < register_info_.size() && register_info_[reg].is_temp;
}

const std::string& RegisterAllocator::GetRegisterName(RegisterIndex reg) const {
    static const std::string empty_name;
    
    if (reg >= register_info_.size()) {
        return empty_name;
    }
    
    return register_info_[reg].name;
}

void RegisterAllocator::SetRegisterName(RegisterIndex reg, const std::string& name) {
    if (reg >= max_registers_) {
        throw CompilerError("Invalid register index: " + std::to_string(reg));
    }
    
    if (reg >= register_info_.size()) {
        register_info_.resize(reg + 1);
    }
    
    register_info_[reg].name = name;
}

RegisterType RegisterAllocator::GetRegisterType(RegisterIndex reg) const {
    if (reg >= register_info_.size()) {
        return RegisterType::Local; // 默认类型
    }
    
    return register_info_[reg].type;
}

void RegisterAllocator::Reset() {
    // 重置所有状态
    std::fill(free_registers_.begin(), free_registers_.end(), true);
    register_info_.clear();
    register_top_ = 0;
    temp_top_ = 0;
    
    // 清空临时标记栈
    while (!temp_markers_.empty()) {
        temp_markers_.pop();
    }
}

void RegisterAllocator::Reserve(Size count) {
    if (count > max_registers_) {
        throw CompilerError("Cannot reserve more registers than maximum: " + std::to_string(count));
    }
    
    // 预分配前 count 个寄存器
    for (Size i = 0; i < count; ++i) {
        if (free_registers_[i]) {
            free_registers_[i] = false;
            
            if (i >= register_info_.size()) {
                register_info_.resize(i + 1);
            }
            register_info_[i] = RegisterInfo{
                RegisterType::Reserved,
                "reserved_" + std::to_string(i),
                static_cast<RegisterIndex>(i),
                false
            };
        }
    }
    
    register_top_ = std::max(register_top_, count);
}

std::vector<RegisterIndex> RegisterAllocator::GetAllocatedRegisters() const {
    std::vector<RegisterIndex> allocated;
    
    for (RegisterIndex i = 0; i < register_top_; ++i) {
        if (!free_registers_[i]) {
            allocated.push_back(i);
        }
    }
    
    return allocated;
}

std::vector<RegisterIndex> RegisterAllocator::GetTemporaryRegisters() const {
    std::vector<RegisterIndex> temps;
    
    for (RegisterIndex i = 0; i < register_top_; ++i) {
        if (i < register_info_.size() && register_info_[i].is_temp) {
            temps.push_back(i);
        }
    }
    
    return temps;
}

std::string RegisterAllocator::ToString() const {
    std::ostringstream oss;
    oss << "RegisterAllocator Status:\n";
    oss << "  Max Registers: " << max_registers_ << "\n";
    oss << "  Register Top: " << register_top_ << "\n";
    oss << "  Temp Top: " << temp_top_ << "\n";
    oss << "  Free Count: " << GetFreeCount() << "\n";
    oss << "  Used Count: " << GetUsedCount() << "\n";
    
    oss << "  Allocated Registers:\n";
    for (RegisterIndex i = 0; i < register_top_; ++i) {
        if (!free_registers_[i]) {
            oss << "    R" << i;
            
            if (i < register_info_.size() && !register_info_[i].name.empty()) {
                oss << " (" << register_info_[i].name << ")";
            }
            
            if (i < register_info_.size() && register_info_[i].is_temp) {
                oss << " [TEMP]";
            }
            
            oss << "\n";
        }
    }
    
    return oss.str();
}

void RegisterAllocator::RecalculateTop() {
    register_top_ = 0;
    temp_top_ = 0;
    
    for (RegisterIndex i = 0; i < max_registers_; ++i) {
        if (!free_registers_[i]) {
            register_top_ = i + 1;
            
            if (i < register_info_.size() && register_info_[i].is_temp) {
                temp_top_ = i + 1;
            }
        }
    }
}

/* ========================================================================== */
/* ScopeManager 实现 */
/* ========================================================================== */

ScopeManager::ScopeManager() : current_level_(0) {
}

ScopeManager::~ScopeManager() = default;

void ScopeManager::EnterScope() {
    scope_markers_.push_back(locals_.size());
    ++current_level_;
}

int ScopeManager::ExitScope() {
    if (scope_markers_.empty()) {
        return 0;
    }
    
    Size marker = scope_markers_.back();
    scope_markers_.pop_back();
    --current_level_;
    
    Size removed_count = locals_.size() - marker;
    locals_.resize(marker);
    
    return static_cast<int>(removed_count);
}

RegisterIndex ScopeManager::DeclareLocal(const std::string& name, RegisterAllocator& allocator) {
    RegisterIndex reg = allocator.AllocateNamed(name);
    
    LocalVariable local;
    local.name = name;
    local.register_idx = reg;
    local.scope_level = current_level_;
    local.is_captured = false;
    
    locals_.push_back(local);
    
    return reg;
}

const LocalVariable* ScopeManager::FindLocal(const std::string& name) const {
    // 从最近声明的变量开始查找（后进先出）
    for (auto it = locals_.rbegin(); it != locals_.rend(); ++it) {
        if (it->name == name) {
            return &(*it);
        }
    }
    return nullptr;
}

RegisterIndex ScopeManager::GetLocalRegister(const std::string& name) const {
    const LocalVariable* local = FindLocal(name);
    return local ? local->register_idx : INVALID_REGISTER;
}

int ScopeManager::GetCurrentLevel() const {
    return current_level_;
}

const std::vector<LocalVariable>& ScopeManager::GetLocals() const {
    return locals_;
}

std::vector<LocalVariable> ScopeManager::GetLocalsInScope(int level) const {
    std::vector<LocalVariable> scope_locals;
    
    for (const auto& local : locals_) {
        if (local.scope_level == level) {
            scope_locals.push_back(local);
        }
    }
    
    return scope_locals;
}

bool ScopeManager::IsLocalDeclared(const std::string& name) const {
    return FindLocal(name) != nullptr;
}

void ScopeManager::MarkCaptured(const std::string& name) {
    for (auto& local : locals_) {
        if (local.name == name) {
            local.is_captured = true;
            break;
        }
    }
}

bool ScopeManager::IsCaptured(const std::string& name) const {
    const LocalVariable* local = FindLocal(name);
    return local && local->is_captured;
}

Size ScopeManager::GetLocalCount() const {
    return locals_.size();
}

void ScopeManager::Clear() {
    locals_.clear();
    scope_markers_.clear();
    current_level_ = 0;
}

std::string ScopeManager::ToString() const {
    std::ostringstream oss;
    oss << "ScopeManager Status:\n";
    oss << "  Current Level: " << current_level_ << "\n";
    oss << "  Total Locals: " << locals_.size() << "\n";
    
    oss << "  Local Variables:\n";
    for (const auto& local : locals_) {
        oss << "    " << local.name << " -> R" << local.register_idx
            << " (level " << local.scope_level << ")";
        
        if (local.is_captured) {
            oss << " [CAPTURED]";
        }
        
        oss << "\n";
    }
    
    return oss.str();
}

/* ========================================================================== */
/* 辅助函数实现 */
/* ========================================================================== */

bool IsValidRegister(RegisterIndex reg) {
    return reg != INVALID_REGISTER && reg <= MAX_REGISTER_INDEX;
}

RegisterIndex NextRegister(RegisterIndex reg) {
    return reg + 1;
}

RegisterIndex PrevRegister(RegisterIndex reg) {
    return (reg > 0) ? (reg - 1) : INVALID_REGISTER;
}

Size CalculateRegisterRange(RegisterIndex start, RegisterIndex end) {
    if (end < start) {
        return 0;
    }
    return static_cast<Size>(end - start + 1);
}

std::string RegisterToString(RegisterIndex reg) {
    if (reg == INVALID_REGISTER) {
        return "INVALID";
    }
    return "R" + std::to_string(reg);
}

} // namespace lua_cpp