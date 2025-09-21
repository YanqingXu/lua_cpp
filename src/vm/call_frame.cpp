/**
 * @file call_frame.cpp
 * @brief Lua调用帧实现
 * @description 实现Lua虚拟机的函数调用栈管理
 * @author Lua C++ Project
 * @date 2025-09-21
 */

#include "call_frame.h"
#include <sstream>

namespace lua_cpp {

/* ========================================================================== */
/* CallFrame构造和析构 */
/* ========================================================================== */

CallFrame::CallFrame(const LuaFunction* function, Size base_stack_index, Size return_count)
    : function_(function)
    , base_stack_index_(base_stack_index)
    , return_count_(return_count)
    , pc_(0)
    , local_count_(0)
    , is_lua_function_(true)
    , is_tail_call_(false)
    , debug_info_{} {
    
    if (function_) {
        local_count_ = function_->max_stack_size;
        debug_info_.function_name = function_->debug_name;
        debug_info_.source = function_->source;
        debug_info_.line_defined = function_->line_defined;
        debug_info_.last_line_defined = function_->last_line_defined;
    }
}

CallFrame::CallFrame(LuaCFunction c_function, Size base_stack_index, Size return_count)
    : function_(nullptr)
    , c_function_(c_function)
    , base_stack_index_(base_stack_index)
    , return_count_(return_count)
    , pc_(0)
    , local_count_(0)
    , is_lua_function_(false)
    , is_tail_call_(false)
    , debug_info_{} {
    
    debug_info_.function_name = "[C function]";
    debug_info_.source = "[C]";
    debug_info_.line_defined = 0;
    debug_info_.last_line_defined = 0;
}

/* ========================================================================== */
/* 访问器方法 */
/* ========================================================================== */

const LuaFunction* CallFrame::GetFunction() const {
    return function_;
}

LuaCFunction CallFrame::GetCFunction() const {
    return c_function_;
}

Size CallFrame::GetBaseStackIndex() const {
    return base_stack_index_;
}

Size CallFrame::GetReturnCount() const {
    return return_count_;
}

Size CallFrame::GetPC() const {
    return pc_;
}

void CallFrame::SetPC(Size pc) {
    pc_ = pc;
    
    // 更新调试信息中的当前行号
    if (function_ && pc < function_->instructions.size()) {
        UpdateCurrentLine();
    }
}

void CallFrame::IncrementPC() {
    pc_++;
    
    if (function_ && pc_ < function_->instructions.size()) {
        UpdateCurrentLine();
    }
}

Size CallFrame::GetLocalCount() const {
    return local_count_;
}

void CallFrame::SetLocalCount(Size count) {
    local_count_ = count;
}

bool CallFrame::IsLuaFunction() const {
    return is_lua_function_;
}

bool CallFrame::IsCFunction() const {
    return !is_lua_function_;
}

/* ========================================================================== */
/* 尾调用支持 */
/* ========================================================================== */

bool CallFrame::IsTailCall() const {
    return is_tail_call_;
}

void CallFrame::SetTailCall(bool is_tail_call) {
    is_tail_call_ = is_tail_call;
}

/* ========================================================================== */
/* 变量管理 */
/* ========================================================================== */

void CallFrame::AddLocal(const std::string& name, Size stack_index, Size start_pc, Size end_pc) {
    LocalVariable local;
    local.name = name;
    local.stack_index = stack_index;
    local.start_pc = start_pc;
    local.end_pc = end_pc;
    
    locals_.push_back(local);
}

const CallFrame::LocalVariable* CallFrame::FindLocal(const std::string& name, Size pc) const {
    for (const auto& local : locals_) {
        if (local.name == name && pc >= local.start_pc && pc < local.end_pc) {
            return &local;
        }
    }
    return nullptr;
}

const std::vector<CallFrame::LocalVariable>& CallFrame::GetLocals() const {
    return locals_;
}

void CallFrame::AddUpvalue(const std::string& name, Size index) {
    UpvalueInfo upvalue;
    upvalue.name = name;
    upvalue.index = index;
    
    upvalues_.push_back(upvalue);
}

const CallFrame::UpvalueInfo* CallFrame::FindUpvalue(const std::string& name) const {
    for (const auto& upvalue : upvalues_) {
        if (upvalue.name == name) {
            return &upvalue;
        }
    }
    return nullptr;
}

const std::vector<CallFrame::UpvalueInfo>& CallFrame::GetUpvalues() const {
    return upvalues_;
}

/* ========================================================================== */
/* 调试信息 */
/* ========================================================================== */

const CallFrame::DebugInfo& CallFrame::GetDebugInfo() const {
    return debug_info_;
}

void CallFrame::SetDebugInfo(const DebugInfo& debug_info) {
    debug_info_ = debug_info;
}

Size CallFrame::GetCurrentLine() const {
    return debug_info_.current_line;
}

void CallFrame::UpdateCurrentLine() {
    if (!function_ || pc_ >= function_->instructions.size()) {
        return;
    }
    
    // 从行号信息中查找当前PC对应的行号
    if (!function_->line_info.empty()) {
        // 二分查找最接近的行号
        Size line = 0;
        for (Size i = 0; i < function_->line_info.size() && i <= pc_; i++) {
            line = function_->line_info[i];
        }
        debug_info_.current_line = line;
    }
}

/* ========================================================================== */
/* 字符串表示 */
/* ========================================================================== */

std::string CallFrame::ToString() const {
    std::ostringstream oss;
    
    oss << "CallFrame[";
    
    if (is_lua_function_) {
        oss << "Lua function: " << debug_info_.function_name;
        if (!debug_info_.source.empty()) {
            oss << " (" << debug_info_.source;
            if (debug_info_.current_line > 0) {
                oss << ":" << debug_info_.current_line;
            }
            oss << ")";
        }
        oss << ", PC=" << pc_;
    } else {
        oss << "C function";
    }
    
    oss << ", base=" << base_stack_index_;
    oss << ", returns=" << return_count_;
    oss << ", locals=" << local_count_;
    
    if (is_tail_call_) {
        oss << ", tail call";
    }
    
    oss << "]";
    
    return oss.str();
}

/* ========================================================================== */
/* CallStack实现 */
/* ========================================================================== */

CallStack::CallStack(Size max_depth)
    : max_depth_(max_depth)
    , current_depth_(0) {
    
    frames_.reserve(VM_INITIAL_CALL_STACK_SIZE);
}

void CallStack::PushFrame(const CallFrame& frame) {
    if (current_depth_ >= max_depth_) {
        throw CallStackOverflowError("Call stack overflow: maximum depth " + std::to_string(max_depth_) + " exceeded");
    }
    
    frames_.push_back(frame);
    current_depth_++;
}

void CallStack::PushFrame(CallFrame&& frame) {
    if (current_depth_ >= max_depth_) {
        throw CallStackOverflowError("Call stack overflow: maximum depth " + std::to_string(max_depth_) + " exceeded");
    }
    
    frames_.push_back(std::move(frame));
    current_depth_++;
}

CallFrame CallStack::PopFrame() {
    if (current_depth_ == 0) {
        throw CallStackUnderflowError("Cannot pop from empty call stack");
    }
    
    CallFrame frame = std::move(frames_.back());
    frames_.pop_back();
    current_depth_--;
    
    return frame;
}

CallFrame& CallStack::CurrentFrame() {
    if (current_depth_ == 0) {
        throw CallStackUnderflowError("No current frame in empty call stack");
    }
    
    return frames_.back();
}

const CallFrame& CallStack::CurrentFrame() const {
    if (current_depth_ == 0) {
        throw CallStackUnderflowError("No current frame in empty call stack");
    }
    
    return frames_.back();
}

CallFrame& CallStack::GetFrame(Size index) {
    if (index >= current_depth_) {
        throw CallStackIndexError("Call stack index out of range: " + std::to_string(index));
    }
    
    return frames_[current_depth_ - 1 - index];  // 0是栈顶
}

const CallFrame& CallStack::GetFrame(Size index) const {
    if (index >= current_depth_) {
        throw CallStackIndexError("Call stack index out of range: " + std::to_string(index));
    }
    
    return frames_[current_depth_ - 1 - index];  // 0是栈顶
}

Size CallStack::Depth() const {
    return current_depth_;
}

Size CallStack::MaxDepth() const {
    return max_depth_;
}

bool CallStack::IsEmpty() const {
    return current_depth_ == 0;
}

bool CallStack::IsFull() const {
    return current_depth_ >= max_depth_;
}

void CallStack::Clear() {
    frames_.clear();
    current_depth_ = 0;
}

std::string CallStack::GetStackTrace() const {
    std::ostringstream oss;
    oss << "Call stack trace (depth=" << current_depth_ << "):\n";
    
    for (Size i = 0; i < current_depth_; i++) {
        oss << "  #" << i << ": " << GetFrame(i).ToString() << "\n";
    }
    
    return oss.str();
}

void CallStack::DumpStackTrace() const {
    std::cout << GetStackTrace() << std::endl;
}

/* ========================================================================== */
/* 异常类实现 */
/* ========================================================================== */

CallStackOverflowError::CallStackOverflowError(const std::string& message)
    : runtime_error(message) {}

CallStackUnderflowError::CallStackUnderflowError(const std::string& message)
    : runtime_error(message) {}

CallStackIndexError::CallStackIndexError(const std::string& message)
    : runtime_error(message) {}

} // namespace lua_cpp