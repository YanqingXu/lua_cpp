#pragma once

#include "core/lua_common.h"
#include "compiler/bytecode.h"
#include "core/lua_errors.h"
#include <memory>
#include <vector>

namespace lua_cpp {

/* ========================================================================== */
/* 前向声明 */
/* ========================================================================== */

class Proto;
class LuaValue;

/* ========================================================================== */
/* 调用帧错误类型 */
/* ========================================================================== */

/**
 * @brief 调用栈溢出错误
 */
class CallStackOverflowError : public LuaError {
public:
    explicit CallStackOverflowError(const std::string& message = "Call stack overflow")
        : LuaError(message, ErrorType::RUNTIME_ERROR) {}
};

/**
 * @brief 调用帧错误
 */
class CallFrameError : public LuaError {
public:
    explicit CallFrameError(const std::string& message = "Call frame error")
        : LuaError(message, ErrorType::RUNTIME_ERROR) {}
};

/* ========================================================================== */
/* 调用帧配置 */
/* ========================================================================== */

/**
 * @brief 调用帧配置常量
 */
constexpr Size VM_MAX_CALL_STACK_DEPTH = 1000;  // 最大调用栈深度
constexpr Size VM_DEFAULT_CALL_STACK_SIZE = 100; // 默认调用栈大小

/* ========================================================================== */
/* 调用帧类 */
/* ========================================================================== */

/**
 * @brief 调用帧类
 * 
 * 表示函数调用的执行上下文，包含：
 * - 函数原型指针
 * - 指令指针
 * - 堆栈基址
 * - 参数和局部变量信息
 * - 返回地址
 */
class CallFrame {
public:
    /**
     * @brief 构造函数
     * @param proto 函数原型
     * @param base 堆栈基址
     * @param param_count 参数数量
     * @param return_address 返回地址（调用点的下一条指令）
     */
    CallFrame(const Proto* proto, Size base, Size param_count, Size return_address = 0);
    
    /**
     * @brief 析构函数
     */
    ~CallFrame() = default;
    
    // 允许拷贝和移动
    CallFrame(const CallFrame&) = default;
    CallFrame& operator=(const CallFrame&) = default;
    CallFrame(CallFrame&&) = default;
    CallFrame& operator=(CallFrame&&) = default;
    
    /* ====================================================================== */
    /* 基本属性访问 */
    /* ====================================================================== */
    
    /**
     * @brief 获取函数原型
     */
    const Proto* GetProto() const { return proto_; }
    
    /**
     * @brief 获取堆栈基址
     */
    Size GetBase() const { return base_; }
    
    /**
     * @brief 获取参数数量
     */
    Size GetParameterCount() const { return param_count_; }
    
    /**
     * @brief 获取返回地址
     */
    Size GetReturnAddress() const { return return_address_; }
    
    /**
     * @brief 设置返回地址
     */
    void SetReturnAddress(Size address) { return_address_ = address; }
    
    /* ====================================================================== */
    /* 指令指针管理 */
    /* ====================================================================== */
    
    /**
     * @brief 获取当前指令指针
     */
    Size GetInstructionPointer() const { return instruction_pointer_; }
    
    /**
     * @brief 设置指令指针
     * @param pc 新的指令指针位置
     */
    void SetInstructionPointer(Size pc);
    
    /**
     * @brief 递增指令指针
     * @param offset 偏移量（默认为1）
     */
    void AdvanceInstructionPointer(int offset = 1);
    
    /**
     * @brief 跳转到指定位置
     * @param target 目标位置
     */
    void JumpTo(Size target);
    
    /**
     * @brief 相对跳转
     * @param offset 跳转偏移量
     */
    void RelativeJump(int offset);
    
    /**
     * @brief 获取当前指令
     * @return 当前指令，如果超出范围返回nullptr
     */
    std::optional<Instruction> GetCurrentInstruction() const;
    
    /**
     * @brief 检查是否到达函数末尾
     */
    bool IsAtEnd() const;
    
    /* ====================================================================== */
    /* 栈管理 */
    /* ====================================================================== */
    
    /**
     * @brief 获取局部变量的堆栈索引
     * @param local_index 局部变量索引
     * @return 堆栈索引
     */
    Size GetLocalStackIndex(Size local_index) const;
    
    /**
     * @brief 获取参数的堆栈索引
     * @param param_index 参数索引
     * @return 堆栈索引
     */
    Size GetParameterStackIndex(Size param_index) const;
    
    /**
     * @brief 获取寄存器的堆栈索引
     * @param register_index 寄存器索引
     * @return 堆栈索引
     */
    Size GetRegisterStackIndex(RegisterIndex register_index) const;
    
    /**
     * @brief 检查寄存器索引是否有效
     * @param register_index 寄存器索引
     * @return 如果有效返回true
     */
    bool IsValidRegister(RegisterIndex register_index) const;
    
    /* ====================================================================== */
    /* 函数信息 */
    /* ====================================================================== */
    
    /**
     * @brief 获取函数名（用于调试）
     */
    std::string GetFunctionName() const;
    
    /**
     * @brief 获取源文件名
     */
    std::string GetSourceName() const;
    
    /**
     * @brief 获取当前行号
     */
    int GetCurrentLine() const;
    
    /**
     * @brief 获取函数定义行号
     */
    int GetDefinitionLine() const;
    
    /**
     * @brief 检查函数是否为可变参数函数
     */
    bool IsVariadic() const;
    
    /* ====================================================================== */
    /* 调试信息 */
    /* ====================================================================== */
    
    /**
     * @brief 调用帧状态结构
     */
    struct FrameInfo {
        const Proto* proto;                 // 函数原型
        Size base;                         // 堆栈基址
        Size instruction_pointer;          // 指令指针
        Size param_count;                  // 参数数量
        Size return_address;               // 返回地址
        std::string function_name;         // 函数名
        std::string source_name;           // 源文件名
        int current_line;                  // 当前行号
        int definition_line;               // 定义行号
        bool is_vararg;                   // 是否可变参数
    };
    
    /**
     * @brief 获取调用帧信息
     */
    FrameInfo GetFrameInfo() const;
    
    /**
     * @brief 获取调用帧的字符串表示
     */
    std::string ToString() const;
    
    /**
     * @brief 获取堆栈跟踪字符串
     */
    std::string GetStackTrace() const;

private:
    /* ====================================================================== */
    /* 成员变量 */
    /* ====================================================================== */
    
    const Proto* proto_;                // 函数原型
    Size base_;                        // 堆栈基址
    Size param_count_;                 // 参数数量
    Size return_address_;              // 返回地址
    Size instruction_pointer_;         // 当前指令指针
};

/* ========================================================================== */
/* 调用栈管理器 */
/* ========================================================================== */

/**
 * @brief 基础调用栈管理器（已废弃，使用 SimpleCallStack 代替）
 * 
 * @deprecated 此类将在未来版本中移除，请使用 vm/simple_call_stack.h 中的 SimpleCallStack
 * 
 * 管理函数调用栈，提供调用帧的创建、销毁和管理功能
 */
class [[deprecated("Use SimpleCallStack from vm/simple_call_stack.h instead")]] BasicCallStack {
public:
    /**
     * @brief 构造函数
     * @param max_depth 最大调用深度
     */
    explicit BasicCallStack(Size max_depth = VM_MAX_CALL_STACK_DEPTH);
    
    /**
     * @brief 析构函数
     */
    ~BasicCallStack() = default;
    
    // 禁用拷贝，允许移动
    BasicCallStack(const BasicCallStack&) = delete;
    BasicCallStack& operator=(const BasicCallStack&) = delete;
    BasicCallStack(BasicCallStack&&) = default;
    BasicCallStack& operator=(BasicCallStack&&) = default;
    
    /* ====================================================================== */
    /* 调用帧操作 */
    /* ====================================================================== */
    
    /**
     * @brief 推入新的调用帧
     * @param proto 函数原型
     * @param base 堆栈基址
     * @param param_count 参数数量
     * @param return_address 返回地址
     * @throws CallStackOverflowError 如果调用栈溢出
     */
    void PushFrame(const Proto* proto, Size base, Size param_count, Size return_address = 0);
    
    /**
     * @brief 弹出当前调用帧
     * @return 弹出的调用帧
     * @throws CallFrameError 如果调用栈为空
     */
    CallFrame PopFrame();
    
    /**
     * @brief 获取当前调用帧
     * @return 当前调用帧的引用
     * @throws CallFrameError 如果调用栈为空
     */
    CallFrame& GetCurrentFrame();
    
    /**
     * @brief 获取当前调用帧（只读）
     * @return 当前调用帧的常量引用
     * @throws CallFrameError 如果调用栈为空
     */
    const CallFrame& GetCurrentFrame() const;
    
    /**
     * @brief 获取指定深度的调用帧
     * @param depth 深度（0为当前帧，1为上一帧）
     * @return 调用帧的引用
     * @throws CallFrameError 如果深度无效
     */
    const CallFrame& GetFrame(Size depth) const;
    
    /* ====================================================================== */
    /* 调用栈状态 */
    /* ====================================================================== */
    
    /**
     * @brief 获取调用栈深度
     */
    Size GetDepth() const { return frames_.size(); }
    
    /**
     * @brief 获取最大深度
     */
    Size GetMaxDepth() const { return max_depth_; }
    
    /**
     * @brief 检查调用栈是否为空
     */
    bool IsEmpty() const { return frames_.empty(); }
    
    /**
     * @brief 检查调用栈是否已满
     */
    bool IsFull() const { return frames_.size() >= max_depth_; }
    
    /**
     * @brief 获取剩余容量
     */
    Size GetAvailableDepth() const { return max_depth_ - frames_.size(); }
    
    /* ====================================================================== */
    /* 批量操作 */
    /* ====================================================================== */
    
    /**
     * @brief 清空调用栈
     */
    void Clear();
    
    /**
     * @brief 获取完整的调用栈跟踪
     */
    std::vector<CallFrame::FrameInfo> GetStackTrace() const;
    
    /**
     * @brief 获取格式化的堆栈跟踪字符串
     * @param max_frames 最多显示的帧数
     */
    std::string FormatStackTrace(Size max_frames = 20) const;
    
    /* ====================================================================== */
    /* 调试和诊断 */
    /* ====================================================================== */
    
    /**
     * @brief 调用栈统计信息
     */
    struct CallStackStats {
        Size current_depth;     // 当前深度
        Size max_depth;         // 最大深度限制
        Size peak_depth;        // 峰值深度
        Size total_calls;       // 总调用次数
        Size total_returns;     // 总返回次数
    };
    
    /**
     * @brief 获取调用栈统计信息
     */
    CallStackStats GetStats() const;
    
    /**
     * @brief 验证调用栈完整性
     */
    bool ValidateIntegrity() const;
    
    /**
     * @brief 获取调用栈的字符串表示
     */
    std::string ToString() const;

private:
    /* ====================================================================== */
    /* 内部方法 */
    /* ====================================================================== */
    
    /**
     * @brief 检查调用栈是否有空间
     * @throws CallStackOverflowError 如果没有空间
     */
    void CheckSpace() const;
    
    /**
     * @brief 检查调用栈是否为空
     * @throws CallFrameError 如果为空
     */
    void CheckNotEmpty() const;
    
    /**
     * @brief 检查深度是否有效
     * @param depth 要检查的深度
     * @throws CallFrameError 如果深度无效
     */
    void CheckDepth(Size depth) const;
    
    /* ====================================================================== */
    /* 成员变量 */
    /* ====================================================================== */
    
    std::vector<CallFrame> frames_;     // 调用帧栈
    Size max_depth_;                    // 最大深度
    
    // 统计信息
    mutable Size peak_depth_;           // 峰值深度
    mutable Size total_calls_;          // 总调用次数
    mutable Size total_returns_;        // 总返回次数
};

/* ========================================================================== */
/* 工厂函数 */
/* ========================================================================== */

/**
 * @brief 创建标准调用栈（已废弃）
 * @deprecated 使用 CreateStandardCallStack() 创建 SimpleCallStack
 * @return 调用栈实例
 */
[[deprecated]] std::unique_ptr<BasicCallStack> CreateStandardBasicCallStack();

/**
 * @brief 创建深度调用栈（用于递归密集的场景）（已废弃）
 * @deprecated 使用 SimpleCallStack 或 AdvancedCallStack
 * @return 调用栈实例
 */
[[deprecated]] std::unique_ptr<BasicCallStack> CreateDeepBasicCallStack();

/**
 * @brief 创建浅层调用栈（用于嵌入式环境）
 * @return 调用栈实例
 */
std::unique_ptr<CallStack> CreateShallowCallStack();

} // namespace lua_cpp