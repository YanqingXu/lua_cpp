/**
 * @file call_stack.h
 * @brief 调用栈抽象接口
 * 
 * 定义调用栈的标准接口，支持多种实现：
 * - SimpleCallStack: 基于 vector 的简单实现
 * - AdvancedCallStack: T026 增强实现（尾调用优化、协程支持）
 * 
 * 设计原则：
 * - 接口分离：VirtualMachine 依赖抽象而非具体实现
 * - 开闭原则：可扩展新实现而不修改现有代码
 * - 依赖注入：派生类可注入不同实现
 * 
 * @author Lua C++ Project Team
 * @date 2025-10-13
 * @version 1.0
 */

#pragma once

#include "call_frame.h"
#include "core/lua_common.h"
#include <vector>

namespace lua_cpp {

/* ========================================================================== */
/* 前向声明 */
/* ========================================================================== */

class Proto;

/* ========================================================================== */
/* CallStack - 调用栈抽象接口 */
/* ========================================================================== */

/**
 * @brief 调用栈抽象基类
 * 
 * 定义调用栈的标准操作接口，所有调用栈实现必须满足此接口。
 * 
 * 线程安全性：实现类负责线程安全
 * 异常安全性：强保证（操作失败时保持原状态）
 */
class CallStack {
public:
    /**
     * @brief 虚析构函数
     */
    virtual ~CallStack() = default;
    
    /* ====================================================================== */
    /* 核心操作 */
    /* ====================================================================== */
    
    /**
     * @brief 推入调用帧
     * 
     * @param proto 函数原型
     * @param base 栈基址
     * @param param_count 参数数量
     * @param return_address 返回地址（可选）
     * @throws std::runtime_error 如果超过最大深度
     * 
     * 复杂度：O(1) 均摊
     * 异常安全：强保证
     */
    virtual void PushFrame(const Proto* proto, 
                          Size base, 
                          Size param_count, 
                          Size return_address = 0) = 0;
    
    /**
     * @brief 弹出调用帧
     * 
     * @return 弹出的调用帧
     * @throws std::logic_error 如果栈为空
     * 
     * 复杂度：O(1)
     * 异常安全：强保证
     */
    virtual CallFrame PopFrame() = 0;
    
    /**
     * @brief 获取当前调用帧（可修改）
     * 
     * @return 当前调用帧的引用
     * @throws std::logic_error 如果栈为空
     * 
     * 复杂度：O(1)
     * 异常安全：不抛出
     */
    virtual CallFrame& GetCurrentFrame() = 0;
    
    /**
     * @brief 获取当前调用帧（只读）
     * 
     * @return 当前调用帧的常量引用
     * @throws std::logic_error 如果栈为空
     * 
     * 复杂度：O(1)
     * 异常安全：不抛出
     */
    virtual const CallFrame& GetCurrentFrame() const = 0;
    
    /* ====================================================================== */
    /* 查询操作 */
    /* ====================================================================== */
    
    /**
     * @brief 获取调用栈深度
     * 
     * @return 当前调用帧数量
     * 
     * 复杂度：O(1)
     * 异常安全：不抛出
     */
    virtual Size GetDepth() const = 0;
    
    /**
     * @brief 检查栈是否为空
     * 
     * @return true 如果栈为空
     * 
     * 复杂度：O(1)
     * 异常安全：不抛出
     */
    virtual bool IsEmpty() const {
        return GetDepth() == 0;
    }
    
    /* ====================================================================== */
    /* 管理操作 */
    /* ====================================================================== */
    
    /**
     * @brief 清空调用栈
     * 
     * 移除所有调用帧，重置为初始状态。
     * 
     * 复杂度：O(n)，n为当前深度
     * 异常安全：不抛出
     */
    virtual void Clear() = 0;
    
    /**
     * @brief 获取最大允许深度
     * 
     * @return 最大调用深度
     * 
     * 复杂度：O(1)
     * 异常安全：不抛出
     */
    virtual Size GetMaxDepth() const = 0;
    
    /* ====================================================================== */
    /* 访问操作（可选，基类提供默认实现） */
    /* ====================================================================== */
    
    /**
     * @brief 获取指定索引的调用帧
     * 
     * @param index 索引（0 = 栈底，GetDepth()-1 = 栈顶）
     * @return 调用帧的常量引用
     * @throws std::out_of_range 如果索引越界
     * 
     * 复杂度：O(1)
     * 异常安全：强保证
     * 
     * 注意：此方法为可选，某些实现可能不支持随机访问
     */
    virtual const CallFrame& GetFrameAt(Size index) const {
        throw std::logic_error("GetFrameAt not supported by this implementation");
    }
    
    /**
     * @brief 获取所有调用帧（用于调试）
     * 
     * @return 所有调用帧的拷贝
     * 
     * 复杂度：O(n)
     * 异常安全：强保证
     * 
     * 注意：此方法主要用于调试和诊断，生产代码应避免使用
     */
    virtual std::vector<CallFrame> GetAllFrames() const {
        std::vector<CallFrame> frames;
        frames.reserve(GetDepth());
        for (Size i = 0; i < GetDepth(); ++i) {
            try {
                frames.push_back(GetFrameAt(i));
            } catch (...) {
                // 实现可能不支持 GetFrameAt
                break;
            }
        }
        return frames;
    }
};

} // namespace lua_cpp
