/**
 * @file simple_call_stack.h
 * @brief 简单调用栈实现
 * 
 * 基于 std::vector 的调用栈实现，用于标准的 VirtualMachine。
 * 提供基本的调用帧管理功能，无尾调用优化或协程支持。
 * 
 * 特性：
 * - 基于 vector 的简单实现
 * - O(1) 均摊时间复杂度
 * - 支持最大深度限制
 * - 线程不安全（由调用者保证）
 * 
 * @author Lua C++ Project Team
 * @date 2025-10-13
 * @version 1.0
 */

#pragma once

#include "call_stack.h"
#include <vector>
#include <stdexcept>

namespace lua_cpp {

/**
 * @brief 简单调用栈实现
 * 
 * 使用 std::vector 实现的基本调用栈，适用于标准 VM。
 * 
 * 性能特性：
 * - PushFrame: O(1) 均摊
 * - PopFrame: O(1)
 * - GetCurrentFrame: O(1)
 * - GetDepth: O(1)
 * 
 * 内存特性：
 * - 自动扩容
 * - 预留最大深度容量避免频繁分配
 */
class SimpleCallStack : public CallStack {
public:
    /**
     * @brief 构造函数
     * 
     * @param max_depth 最大调用深度（默认：VM_MAX_CALL_STACK_DEPTH）
     */
    explicit SimpleCallStack(Size max_depth = VM_MAX_CALL_STACK_DEPTH);
    
    /**
     * @brief 析构函数
     */
    ~SimpleCallStack() override = default;
    
    // 禁用拷贝，允许移动
    SimpleCallStack(const SimpleCallStack&) = delete;
    SimpleCallStack& operator=(const SimpleCallStack&) = delete;
    SimpleCallStack(SimpleCallStack&&) noexcept = default;
    SimpleCallStack& operator=(SimpleCallStack&&) noexcept = default;
    
    /* ====================================================================== */
    /* CallStack 接口实现 */
    /* ====================================================================== */
    
    void PushFrame(const Proto* proto, 
                  Size base, 
                  Size param_count, 
                  Size return_address = 0) override;
    
    CallFrame PopFrame() override;
    
    CallFrame& GetCurrentFrame() override;
    const CallFrame& GetCurrentFrame() const override;
    
    Size GetDepth() const override;
    
    void Clear() override;
    
    Size GetMaxDepth() const override;
    
    const CallFrame& GetFrameAt(Size index) const override;
    
private:
    /**
     * @brief 检查栈是否为空
     * @throws std::logic_error 如果栈为空
     */
    void CheckNotEmpty() const;
    
    /**
     * @brief 检查是否超过最大深度
     * @throws std::runtime_error 如果超过最大深度
     */
    void CheckDepth() const;
    
    std::vector<CallFrame> frames_;  ///< 调用帧存储
    Size max_depth_;                 ///< 最大深度限制
};

} // namespace lua_cpp
