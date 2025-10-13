/**
 * @file simple_call_stack.cpp
 * @brief 简单调用栈实现
 * 
 * @author Lua C++ Project Team
 * @date 2025-10-13
 */

#include "simple_call_stack.h"
#include <sstream>

namespace lua_cpp {

/* ========================================================================== */
/* 构造与析构 */
/* ========================================================================== */

SimpleCallStack::SimpleCallStack(Size max_depth)
    : max_depth_(max_depth) {
    // 预留容量避免频繁分配
    frames_.reserve(max_depth);
}

/* ========================================================================== */
/* 核心操作 */
/* ========================================================================== */

void SimpleCallStack::PushFrame(const Proto* proto, 
                               Size base, 
                               Size param_count, 
                               Size return_address) {
    // 检查深度限制
    CheckDepth();
    
    // 创建并推入新帧
    CallFrame frame;
    frame.proto = proto;
    frame.base = base;
    frame.param_count = param_count;
    frame.return_address = return_address;
    frame.pc = 0;
    
    frames_.push_back(frame);
}

CallFrame SimpleCallStack::PopFrame() {
    CheckNotEmpty();
    
    CallFrame frame = frames_.back();
    frames_.pop_back();
    
    return frame;
}

CallFrame& SimpleCallStack::GetCurrentFrame() {
    CheckNotEmpty();
    return frames_.back();
}

const CallFrame& SimpleCallStack::GetCurrentFrame() const {
    CheckNotEmpty();
    return frames_.back();
}

/* ========================================================================== */
/* 查询操作 */
/* ========================================================================== */

Size SimpleCallStack::GetDepth() const {
    return static_cast<Size>(frames_.size());
}

Size SimpleCallStack::GetMaxDepth() const {
    return max_depth_;
}

const CallFrame& SimpleCallStack::GetFrameAt(Size index) const {
    if (index >= frames_.size()) {
        std::ostringstream oss;
        oss << "CallStack index out of range: " << index 
            << " >= " << frames_.size();
        throw std::out_of_range(oss.str());
    }
    return frames_[index];
}

/* ========================================================================== */
/* 管理操作 */
/* ========================================================================== */

void SimpleCallStack::Clear() {
    frames_.clear();
}

/* ========================================================================== */
/* 辅助方法 */
/* ========================================================================== */

void SimpleCallStack::CheckNotEmpty() const {
    if (frames_.empty()) {
        throw std::logic_error("CallStack is empty");
    }
}

void SimpleCallStack::CheckDepth() const {
    if (frames_.size() >= max_depth_) {
        std::ostringstream oss;
        oss << "CallStack overflow: depth " << frames_.size() 
            << " >= max " << max_depth_;
        throw std::runtime_error(oss.str());
    }
}

} // namespace lua_cpp
