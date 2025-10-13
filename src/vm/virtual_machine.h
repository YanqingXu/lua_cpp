#pragma once

#include "stack.h"
#include "call_frame.h"
#include "compiler/bytecode.h"
#include "core/lua_common.h"
#include "types/value.h"
#include "core/lua_errors.h"
#include <memory>
#include <vector>
#include <array>
#include <functional>
#include <unordered_map>

namespace lua_cpp {

/* ========================================================================== */
/* 前向声明 */
/* ========================================================================== */

class Proto;
class LuaTable;

/* ========================================================================== */
/* VM错误类型 */
/* ========================================================================== */

/**
 * @brief 虚拟机执行错误
 */
class VMExecutionError : public LuaError {
public:
    explicit VMExecutionError(const std::string& message = "VM execution error")
        : LuaError(message, ErrorType::RUNTIME_ERROR) {}
};

/**
 * @brief 无效指令错误
 */
class InvalidInstructionError : public LuaError {
public:
    explicit InvalidInstructionError(const std::string& message = "Invalid instruction")
        : LuaError(message, ErrorType::RUNTIME_ERROR) {}
};

/**
 * @brief 运行时错误
 */
class RuntimeError : public LuaError {
public:
    explicit RuntimeError(const std::string& message = "Runtime error")
        : LuaError(message, ErrorType::RUNTIME_ERROR) {}
};

/* ========================================================================== */
/* VM执行状态 */
/* ========================================================================== */

/**
 * @brief 虚拟机执行状态
 */
enum class ExecutionState {
    Ready,          // 就绪状态
    Running,        // 运行状态
    Suspended,      // 暂停状态
    Error,          // 错误状态
    Finished        // 完成状态
};

/* ========================================================================== */
/* VM配置 */
/* ========================================================================== */

/**
 * @brief 虚拟机配置
 */
struct VMConfig {
    // 堆栈配置
    Size initial_stack_size = VM_DEFAULT_STACK_SIZE;   // 初始堆栈大小
    Size max_stack_size = VM_MAX_STACK_SIZE;           // 最大堆栈大小
    
    // 调用栈配置
    Size max_call_depth = VM_MAX_CALL_STACK_DEPTH;     // 最大调用深度
    
    // 调试配置
    bool enable_debug_info = false;                    // 启用调试信息
    bool enable_profiling = false;                     // 启用性能分析
    bool enable_stack_trace = true;                    // 启用堆栈跟踪
    
    // 执行配置
    Size max_instructions_per_step = 1000;             // 每步最大指令数
    bool enable_instruction_limit = false;             // 启用指令限制
    Size instruction_limit = 1000000;                  // 指令执行限制
    
    // 内存配置
    Size gc_threshold = 1024 * 1024;                   // GC触发阈值
    bool enable_auto_gc = true;                        // 启用自动GC
};

/* ========================================================================== */
/* 调试信息 */
/* ========================================================================== */

/**
 * @brief 调试信息结构
 */
struct DebugInfo {
    Size instruction_pointer;           // 指令指针
    const Proto* current_function;      // 当前函数
    OpCode current_opcode;             // 当前操作码
    Instruction current_instruction;    // 当前指令
    int current_line;                  // 当前行号
    std::string source_name;           // 源文件名
    std::string function_name;         // 函数名
};

/**
 * @brief 调试钩子类型
 */
using DebugHook = std::function<void(const DebugInfo&)>;

/* ========================================================================== */
/* 执行统计 */
/* ========================================================================== */

/**
 * @brief 执行统计信息
 */
struct ExecutionStatistics {
    Size total_instructions = 0;                               // 总指令数
    std::array<Size, static_cast<int>(OpCode::NUM_OPCODES)> 
        instruction_counts = {};                                // 各指令执行次数
    Size function_calls = 0;                                   // 函数调用次数
    Size table_operations = 0;                                 // 表操作次数
    Size gc_collections = 0;                                   // GC收集次数
    double execution_time = 0.0;                               // 执行时间（秒）
    Size peak_stack_usage = 0;                                 // 峰值堆栈使用
    Size peak_call_depth = 0;                                  // 峰值调用深度
};

/* ========================================================================== */
/* 虚拟机主类 */
/* ========================================================================== */

/**
 * @brief Lua虚拟机类
 * 
 * 实现Lua 5.1.5的虚拟机，提供字节码执行功能
 * 主要功能：
 * - 字节码指令执行
 * - 堆栈和调用帧管理
 * - 错误处理和异常恢复
 * - 调试和性能分析
 * - 程序完整执行
 */
class VirtualMachine {
public:
    /**
     * @brief 构造函数
     * @param config 虚拟机配置
     */
    explicit VirtualMachine(const VMConfig& config = VMConfig());
    
    /**
     * @brief 析构函数
     */
    ~VirtualMachine() = default;
    
    // 禁用拷贝，允许移动
    VirtualMachine(const VirtualMachine&) = delete;
    VirtualMachine& operator=(const VirtualMachine&) = delete;
    VirtualMachine(VirtualMachine&&) = default;
    VirtualMachine& operator=(VirtualMachine&&) = default;
    
    /* ====================================================================== */
    /* 执行控制 */
    /* ====================================================================== */
    
    /**
     * @brief 执行完整程序
     * @param proto 主函数原型
     * @param args 程序参数
     * @return 程序返回值
     */
    std::vector<LuaValue> ExecuteProgram(const Proto* proto, 
                                        const std::vector<LuaValue>& args = {});
    
    /**
     * @brief 执行单条指令
     * @param instruction 要执行的指令
     */
    void ExecuteInstruction(Instruction instruction);
    
    /**
     * @brief 执行多条指令
     * @param max_instructions 最大指令数（0表示无限制）
     * @return 实际执行的指令数
     */
    Size ExecuteInstructions(Size max_instructions = 0);
    
    /**
     * @brief 单步执行
     * @return 是否还有更多指令要执行
     */
    bool StepExecution();
    
    /**
     * @brief 继续执行直到完成或错误
     */
    void ContinueExecution();
    
    /**
     * @brief 暂停执行
     */
    void Suspend();
    
    /**
     * @brief 重置虚拟机到初始状态
     */
    void Reset();
    
    /* ====================================================================== */
    /* 状态管理 */
    /* ====================================================================== */
    
    /**
     * @brief 获取执行状态
     */
    ExecutionState GetExecutionState() const { return execution_state_; }
    
    /**
     * @brief 设置执行状态
     */
    void SetExecutionState(ExecutionState state) { execution_state_ = state; }
    
    /**
     * @brief 获取指令指针
     */
    Size GetInstructionPointer() const;
    
    /**
     * @brief 设置指令指针
     */
    void SetInstructionPointer(Size pc);
    
    /**
     * @brief 检查是否正在运行
     */
    bool IsRunning() const { return execution_state_ == ExecutionState::Running; }
    
    /**
     * @brief 检查是否已完成
     */
    bool IsFinished() const { return execution_state_ == ExecutionState::Finished; }
    
    /**
     * @brief 检查是否有错误
     */
    bool HasError() const { return execution_state_ == ExecutionState::Error; }
    
    /* ====================================================================== */
    /* 堆栈操作 */
    /* ====================================================================== */
    
    /**
     * @brief 推入值到堆栈
     */
    void Push(const LuaValue& value) { stack_->Push(value); }
    void Push(LuaValue&& value) { stack_->Push(std::move(value)); }
    
    /**
     * @brief 弹出堆栈顶部值
     */
    LuaValue Pop() { return stack_->Pop(); }
    
    /**
     * @brief 获取堆栈顶部值
     */
    const LuaValue& Top() const { return stack_->Top(); }
    LuaValue& Top() { return stack_->Top(); }
    
    /**
     * @brief 获取堆栈中的值
     */
    const LuaValue& GetStack(Size index) const { return stack_->Get(index); }
    LuaValue& GetStack(Size index) { return stack_->Get(index); }
    
    /**
     * @brief 设置堆栈中的值
     */
    void SetStack(Size index, const LuaValue& value) { stack_->Set(index, value); }
    void SetStack(Size index, LuaValue&& value) { stack_->Set(index, std::move(value)); }
    
    /**
     * @brief 获取堆栈大小
     */
    Size GetStackSize() const { return stack_->GetCapacity(); }
    
    /**
     * @brief 获取堆栈顶部位置
     */
    Size GetStackTop() const { return stack_->GetTop(); }
    
    /**
     * @brief 设置堆栈顶部位置
     */
    void SetStackTop(Size top) { stack_->SetTop(top); }
    
    /**
     * @brief 获取最大堆栈大小
     */
    Size GetMaxStackSize() const { return stack_->GetMaxSize(); }
    
    /* ====================================================================== */
    /* 调用帧操作 */
    /* ====================================================================== */
    
    /**
     * @brief 推入调用帧
     */
    void PushCallFrame(const Proto* proto, Size base, Size param_count, Size return_address = 0) {
        call_stack_->PushFrame(proto, base, param_count, return_address);
    }
    
    /**
     * @brief 获取当前调用帧
     */
    CallFrame& GetCurrentCallFrame() { return call_stack_->GetCurrentFrame(); }
    const CallFrame& GetCurrentCallFrame() const { return call_stack_->GetCurrentFrame(); }
    
    /**
     * @brief 获取调用帧数量
     */
    Size GetCallFrameCount() const { return call_stack_->GetDepth(); }
    
    /* ====================================================================== */
    /* 配置访问 */
    /* ====================================================================== */
    
    /**
     * @brief 获取VM配置
     */
    const VMConfig& GetConfig() const { return config_; }
    
    /**
     * @brief 检查是否启用调试
     */
    bool IsDebugEnabled() const { return config_.enable_debug_info; }
    
    /**
     * @brief 检查是否启用性能分析
     */
    bool IsProfilingEnabled() const { return config_.enable_profiling; }
    
    /**
     * @brief 设置调试钩子
     */
    void SetDebugHook(const DebugHook& hook) { debug_hook_ = hook; }
    
    /**
     * @brief 清除调试钩子
     */
    void ClearDebugHook() { debug_hook_ = nullptr; }
    
    /* ====================================================================== */
    /* 统计和诊断 */
    /* ====================================================================== */
    
    /**
     * @brief 获取执行统计信息
     */
    const ExecutionStatistics& GetExecutionStatistics() const { return statistics_; }
    
    /**
     * @brief 重置统计信息
     */
    void ResetStatistics();
    
    /**
     * @brief 获取内存使用量
     */
    Size GetMemoryUsage() const;
    
    /**
     * @brief 获取调试信息
     */
    DebugInfo GetCurrentDebugInfo() const;
    
    /**
     * @brief 获取堆栈跟踪
     */
    std::string GetStackTrace() const;

private:
    /* ====================================================================== */
    /* 指令执行方法 */
    /* ====================================================================== */
    
    // 数据移动指令
    void ExecuteMOVE(RegisterIndex a, int b);
    void ExecuteLOADK(RegisterIndex a, int bx);
    void ExecuteLOADBOOL(RegisterIndex a, int b, int c);
    void ExecuteLOADNIL(RegisterIndex a, int b);
    
    // 全局变量和上值指令
    void ExecuteGETUPVAL(RegisterIndex a, int b);
    void ExecuteGETGLOBAL(RegisterIndex a, int bx);
    void ExecuteGETTABLE(RegisterIndex a, int b, int c);
    void ExecuteSETGLOBAL(RegisterIndex a, int bx);
    void ExecuteSETUPVAL(RegisterIndex a, int b);
    void ExecuteSETTABLE(RegisterIndex a, int b, int c);
    
    // 表操作指令
    void ExecuteNEWTABLE(RegisterIndex a, int b, int c);
    void ExecuteSELF(RegisterIndex a, int b, int c);
    void ExecuteSETLIST(RegisterIndex a, int b, int c);
    
    // 算术指令
    void ExecuteADD(RegisterIndex a, int b, int c);
    void ExecuteSUB(RegisterIndex a, int b, int c);
    void ExecuteMUL(RegisterIndex a, int b, int c);
    void ExecuteDIV(RegisterIndex a, int b, int c);
    void ExecuteMOD(RegisterIndex a, int b, int c);
    void ExecutePOW(RegisterIndex a, int b, int c);
    void ExecuteUNM(RegisterIndex a, int b);
    void ExecuteNOT(RegisterIndex a, int b);
    void ExecuteLEN(RegisterIndex a, int b);
    void ExecuteCONCAT(RegisterIndex a, int b, int c);
    
    // 跳转和比较指令
    void ExecuteJMP(int sbx);
    void ExecuteEQ(RegisterIndex a, int b, int c);
    void ExecuteLT(RegisterIndex a, int b, int c);
    void ExecuteLE(RegisterIndex a, int b, int c);
    void ExecuteTEST(RegisterIndex a, int c);
    void ExecuteTESTSET(RegisterIndex a, int b, int c);
    
    // 函数调用指令
    void ExecuteCALL(RegisterIndex a, int b, int c);
    void ExecuteTAILCALL(RegisterIndex a, int b, int c);
    void ExecuteRETURN(RegisterIndex a, int b);
    
    // 循环指令
    void ExecuteFORLOOP(RegisterIndex a, int sbx);
    void ExecuteFORPREP(RegisterIndex a, int sbx);
    void ExecuteTFORLOOP(RegisterIndex a, int c);
    
    // 闭包和其他指令
    void ExecuteCLOSE(RegisterIndex a);
    void ExecuteCLOSURE(RegisterIndex a, int bx);
    void ExecuteVARARG(RegisterIndex a, int b);
    
    /* ====================================================================== */
    /* 虚拟机内部方法 */
    /* ====================================================================== */
    
    /**
     * @brief 检查是否还有更多指令
     */
    bool HasMoreInstructions() const;
    
    /**
     * @brief 获取下一条指令
     */
    Instruction GetNextInstruction() const;
    
    /**
     * @brief 获取当前行号
     */
    int GetCurrentLine() const;
    
    /**
     * @brief 设置寄存器值
     */
    void SetRegister(RegisterIndex reg, const LuaValue& value);
    
    /**
     * @brief 获取寄存器值
     */
    LuaValue GetRegister(RegisterIndex reg) const;
    
    /**
     * @brief 获取RK值（寄存器或常量）
     */
    LuaValue GetRK(int rk) const;
    
    /**
     * @brief 获取当前调用帧的基址
     */
    Size GetCurrentBase() const;
    
    /**
     * @brief 推入调用帧
     */
    void PushCallFrame(const Proto* proto, Size base, Size param_count);
    
    /**
     * @brief 弹出调用帧
     */
    void PopCallFrame();
    
    /* ====================================================================== */
    /* 指令解码方法 */
    /* ====================================================================== */
    
    OpCode DecodeOpCode(Instruction inst) const;
    RegisterIndex DecodeA(Instruction inst) const;
    int DecodeB(Instruction inst) const;
    int DecodeC(Instruction inst) const;
    int DecodeBx(Instruction inst) const;
    int DecodeSBx(Instruction inst) const;
    
    /* ====================================================================== */
    /* 成员变量 */
    /* ====================================================================== */
    
    // 配置
    VMConfig config_;
    
    // 核心组件
    std::unique_ptr<LuaStack> stack_;           // 值堆栈
    std::vector<CallFrame> call_stack_;         // 调用栈
    
    // 执行状态
    ExecutionState execution_state_;            // 执行状态
    Size instruction_pointer_;                  // 指令指针
    const Proto* current_proto_;                // 当前函数原型
    
    // 全局状态
    std::shared_ptr<LuaTable> global_table_;    // 全局变量表
    
    // 调试和分析
    DebugHook debug_hook_;                      // 调试钩子
    ExecutionStatistics statistics_;           // 执行统计
    Size instruction_count_;                    // 指令计数器
};

/* ========================================================================== */
/* 工厂函数 */
/* ========================================================================== */

/**
 * @brief 创建标准虚拟机
 */
std::unique_ptr<VirtualMachine> CreateStandardVM();

/**
 * @brief 创建调试虚拟机
 */
std::unique_ptr<VirtualMachine> CreateDebugVM();

/**
 * @brief 创建高性能虚拟机
 */
std::unique_ptr<VirtualMachine> CreateHighPerformanceVM();

/**
 * @brief 创建嵌入式虚拟机
 */
std::unique_ptr<VirtualMachine> CreateEmbeddedVM();

} // namespace lua_cpp