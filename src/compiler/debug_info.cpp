/**
 * @file debug_info.cpp
 * @brief 调试信息生成器实现
 * @description 实现调试信息的生成和管理功能
 * @author Lua C++ Project
 * @date 2025-09-21
 */

#include "debug_info.h"
#include <algorithm>
#include <sstream>

namespace lua_cpp {

/* ========================================================================== */
/* 调试信息生成器实现 */
/* ========================================================================== */

void DebugInfoGenerator::BeginFunction(const std::string& name, 
                                       const std::string& source_name,
                                       int line_defined) {
    current_function_ = FunctionDebugInfo(name, source_name);
    current_function_.line_defined = line_defined;
    temp_locals_.clear();
    source_locations_.clear();
}

FunctionDebugInfo DebugInfoGenerator::EndFunction(int last_line_defined) {
    current_function_.last_line_defined = last_line_defined;
    
    // 将临时局部变量信息移动到函数信息中
    current_function_.locals = std::move(temp_locals_);
    temp_locals_.clear();
    
    return std::move(current_function_);
}

/* ===== 行号信息管理 ===== */

void DebugInfoGenerator::SetLineInfo(int pc, int line) {
    ResizeLineInfo(pc + 1);
    current_function_.line_info[pc] = line;
}

void DebugInfoGenerator::SetLineInfo(const std::vector<int>& line_info) {
    current_function_.line_info = line_info;
}

int DebugInfoGenerator::GetLineInfo(int pc) const {
    if (pc >= 0 && pc < current_function_.line_info.size()) {
        return current_function_.line_info[pc];
    }
    return 0;
}

/* ===== 局部变量信息管理 ===== */

void DebugInfoGenerator::RegisterLocal(const std::string& name, 
                                       int start_pc, 
                                       RegisterIndex register_idx) {
    temp_locals_.emplace_back(name, start_pc, -1, register_idx);
}

void DebugInfoGenerator::EndLocal(const std::string& name, int end_pc) {
    int index = FindLocalIndex(name);
    if (index >= 0) {
        temp_locals_[index].end_pc = end_pc;
    }
}

void DebugInfoGenerator::EndLocals(int end_pc, int count) {
    // 从最后开始结束指定数量的局部变量
    int start_index = std::max(0, static_cast<int>(temp_locals_.size()) - count);
    for (int i = start_index; i < temp_locals_.size(); ++i) {
        if (temp_locals_[i].end_pc == -1) {
            temp_locals_[i].end_pc = end_pc;
        }
    }
}

std::vector<LocalDebugInfo> DebugInfoGenerator::GetLocalsAtPC(int pc) const {
    std::vector<LocalDebugInfo> result;
    
    // 检查当前函数的局部变量
    for (const auto& local : current_function_.locals) {
        if (pc >= local.start_pc && (local.end_pc == -1 || pc < local.end_pc)) {
            result.push_back(local);
        }
    }
    
    // 检查临时局部变量
    for (const auto& local : temp_locals_) {
        if (pc >= local.start_pc && (local.end_pc == -1 || pc < local.end_pc)) {
            result.push_back(local);
        }
    }
    
    return result;
}

/* ===== 上值信息管理 ===== */

void DebugInfoGenerator::RegisterUpvalue(const std::string& name, 
                                          bool in_stack, 
                                          int index) {
    current_function_.upvalues.emplace_back(name, in_stack, index);
}

const UpvalueDebugInfo* DebugInfoGenerator::GetUpvalueInfo(int index) const {
    if (index >= 0 && index < current_function_.upvalues.size()) {
        return &current_function_.upvalues[index];
    }
    return nullptr;
}

/* ===== 源码位置映射 ===== */

void DebugInfoGenerator::SetSourceLocation(int pc, const SourceLocation& location) {
    source_locations_[pc] = location;
}

SourceLocation DebugInfoGenerator::GetSourceLocation(int pc) const {
    auto it = source_locations_.find(pc);
    if (it != source_locations_.end()) {
        return it->second;
    }
    return SourceLocation();
}

/* ===== 调试信息查询 ===== */

const FunctionDebugInfo& DebugInfoGenerator::GetCurrentFunctionInfo() const {
    return current_function_;
}

const LocalDebugInfo* DebugInfoGenerator::FindLocal(const std::string& name, int pc) const {
    // 先在当前函数的局部变量中查找
    for (const auto& local : current_function_.locals) {
        if (local.name == name && 
            pc >= local.start_pc && 
            (local.end_pc == -1 || pc < local.end_pc)) {
            return &local;
        }
    }
    
    // 再在临时局部变量中查找
    for (const auto& local : temp_locals_) {
        if (local.name == name && 
            pc >= local.start_pc && 
            (local.end_pc == -1 || pc < local.end_pc)) {
            return &local;
        }
    }
    
    return nullptr;
}

void DebugInfoGenerator::Clear() {
    current_function_ = FunctionDebugInfo();
    temp_locals_.clear();
    source_locations_.clear();
}

/* ===== 辅助方法 ===== */

int DebugInfoGenerator::FindLocalIndex(const std::string& name) const {
    // 从后向前查找，因为可能有同名变量
    for (int i = static_cast<int>(temp_locals_.size()) - 1; i >= 0; --i) {
        if (temp_locals_[i].name == name && temp_locals_[i].end_pc == -1) {
            return i;
        }
    }
    return -1;
}

void DebugInfoGenerator::ResizeLineInfo(Size size) {
    if (current_function_.line_info.size() < size) {
        current_function_.line_info.resize(size, 0);
    }
}

/* ========================================================================== */
/* 调试信息工具函数 */
/* ========================================================================== */

std::string FormatSourceLocation(const SourceLocation& location) {
    if (location.line == 0) {
        return "unknown";
    }
    
    std::ostringstream oss;
    oss << "line " << location.line;
    if (location.column > 0) {
        oss << ", column " << location.column;
    }
    return oss.str();
}

std::string FormatLocalInfo(const LocalDebugInfo& local) {
    std::ostringstream oss;
    oss << "local '" << local.name << "' (register " << local.register_idx 
        << ", pc " << local.start_pc;
    if (local.end_pc != -1) {
        oss << "-" << local.end_pc;
    } else {
        oss << "+";
    }
    oss << ")";
    return oss.str();
}

std::string FormatFunctionInfo(const FunctionDebugInfo& info) {
    std::ostringstream oss;
    
    if (info.name.empty()) {
        oss << "anonymous function";
    } else {
        oss << "function '" << info.name << "'";
    }
    
    if (!info.source_name.empty()) {
        oss << " in " << info.source_name;
    }
    
    if (info.line_defined > 0) {
        oss << " at line " << info.line_defined;
        if (info.last_line_defined > info.line_defined) {
            oss << "-" << info.last_line_defined;
        }
    }
    
    oss << "\n";
    oss << "  " << info.locals.size() << " local variables\n";
    oss << "  " << info.upvalues.size() << " upvalues\n";
    oss << "  " << info.line_info.size() << " instructions";
    
    return oss.str();
}

} // namespace lua_cpp