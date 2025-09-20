/**
 * @file vm-interface.h  
 * @brief 虚拟机核心接口契约
 * @date 2025-09-20
 */

#ifndef VM_INTERFACE_H
#define VM_INTERFACE_H

#include <memory>
#include <vector>
#include <optional>

namespace lua_vm {

/* ========================================================================== */
/* 前向声明 */
/* ========================================================================== */

class LuaValue;
class LuaState;
class LuaChunk;
class Instruction;

/* ========================================================================== */
/* 虚拟机核心接口 */
/* ========================================================================== */

/**
 * @brief 虚拟机执行器接口
 */
class IVirtualMachine {
public:
    virtual ~IVirtualMachine() = default;
    
    /**
     * @brief 执行字节码块
     * @param state Lua状态机
     * @param chunk 字节码块
     * @return 执行结果
     */
    virtual ExecutionResult execute(LuaState& state, 
                                  std::shared_ptr<LuaChunk> chunk) = 0;
    
    /**
     * @brief 单步执行指令
     * @param state Lua状态机
     * @param instruction 指令
     * @return 执行结果
     */
    virtual InstructionResult execute_instruction(LuaState& state,
                                                const Instruction& instruction) = 0;
    
    /**
     * @brief 设置断点
     * @param chunk 字节码块
     * @param pc 程序计数器位置
     */
    virtual void set_breakpoint(std::shared_ptr<LuaChunk> chunk, size_t pc) = 0;
    
    /**
     * @brief 移除断点
     * @param chunk 字节码块
     * @param pc 程序计数器位置
     */
    virtual void remove_breakpoint(std::shared_ptr<LuaChunk> chunk, size_t pc) = 0;
};

/* ========================================================================== */
/* 指令分发器接口 */
/* ========================================================================== */

/**
 * @brief 指令分发器接口
 */
class IInstructionDispatcher {
public:
    virtual ~IInstructionDispatcher() = default;
    
    /**
     * @brief 分发指令执行
     * @param state Lua状态机
     * @param instruction 指令
     * @return 执行结果
     */
    virtual InstructionResult dispatch(LuaState& state,
                                     const Instruction& instruction) = 0;
    
    /**
     * @brief 注册指令处理器
     * @param opcode 操作码
     * @param handler 处理器函数
     */
    virtual void register_handler(uint8_t opcode, InstructionHandler handler) = 0;
    
    /**
     * @brief 获取指令执行统计
     * @return 指令执行次数统计
     */
    virtual std::vector<uint64_t> get_instruction_stats() const = 0;
};

/* ========================================================================== */
/* 执行栈接口 */
/* ========================================================================== */

/**
 * @brief 执行栈接口
 */
class IExecutionStack {
public:
    virtual ~IExecutionStack() = default;
    
    /**
     * @brief 压入值
     * @param value 要压入的值
     */
    virtual void push(const LuaValue& value) = 0;
    
    /**
     * @brief 弹出值
     * @return 弹出的值
     */
    virtual LuaValue pop() = 0;
    
    /**
     * @brief 获取栈顶值
     * @param offset 偏移量（0表示栈顶）
     * @return 栈顶值的引用
     */
    virtual LuaValue& top(int offset = 0) = 0;
    
    /**
     * @brief 获取栈大小
     * @return 栈大小
     */
    virtual size_t size() const = 0;
    
    /**
     * @brief 检查栈空间
     * @param required 需要的空间大小
     * @return 是否有足够空间
     */
    virtual bool check_space(size_t required) const = 0;
    
    /**
     * @brief 调整栈大小
     * @param new_top 新的栈顶位置
     */
    virtual void set_top(size_t new_top) = 0;
};

/* ========================================================================== */
/* 调用帧接口 */
/* ========================================================================== */

/**
 * @brief 调用帧信息
 */
struct CallFrame {
    std::shared_ptr<LuaChunk> chunk;    ///< 字节码块
    size_t pc;                          ///< 程序计数器
    size_t stack_base;                  ///< 栈基址
    size_t num_params;                  ///< 参数个数
    size_t num_results;                 ///< 期望返回值个数
    bool is_vararg;                     ///< 是否可变参数
};

/**
 * @brief 调用栈接口
 */
class ICallStack {
public:
    virtual ~ICallStack() = default;
    
    /**
     * @brief 压入调用帧
     * @param frame 调用帧
     */
    virtual void push_frame(const CallFrame& frame) = 0;
    
    /**
     * @brief 弹出调用帧
     * @return 弹出的调用帧
     */
    virtual CallFrame pop_frame() = 0;
    
    /**
     * @brief 获取当前调用帧
     * @return 当前调用帧的引用
     */
    virtual CallFrame& current_frame() = 0;
    
    /**
     * @brief 获取调用栈深度
     * @return 调用栈深度
     */
    virtual size_t depth() const = 0;
    
    /**
     * @brief 获取调用栈跟踪
     * @return 调用栈跟踪信息
     */
    virtual std::vector<CallFrame> get_traceback() const = 0;
};

/* ========================================================================== */
/* 指令定义和类型 */
/* ========================================================================== */

/**
 * @brief Lua操作码枚举
 */
enum class OpCode : uint8_t {
    OP_MOVE = 0,      ///< 寄存器复制
    OP_LOADK,         ///< 加载常量
    OP_LOADBOOL,      ///< 加载布尔值
    OP_LOADNIL,       ///< 加载nil
    OP_GETUPVAL,      ///< 获取上值
    OP_GETGLOBAL,     ///< 获取全局变量
    OP_GETTABLE,      ///< 获取表字段
    OP_SETGLOBAL,     ///< 设置全局变量
    OP_SETUPVAL,      ///< 设置上值
    OP_SETTABLE,      ///< 设置表字段
    OP_NEWTABLE,      ///< 创建新表
    OP_SELF,          ///< 获取方法
    OP_ADD,           ///< 加法
    OP_SUB,           ///< 减法
    OP_MUL,           ///< 乘法
    OP_DIV,           ///< 除法
    OP_MOD,           ///< 模运算
    OP_POW,           ///< 幂运算
    OP_UNM,           ///< 取负数
    OP_NOT,           ///< 逻辑非
    OP_LEN,           ///< 长度运算
    OP_CONCAT,        ///< 字符串连接
    OP_JMP,           ///< 跳转
    OP_EQ,            ///< 等于比较
    OP_LT,            ///< 小于比较
    OP_LE,            ///< 小于等于比较
    OP_TEST,          ///< 测试
    OP_TESTSET,       ///< 测试并设置
    OP_CALL,          ///< 函数调用
    OP_TAILCALL,      ///< 尾调用
    OP_RETURN,        ///< 返回
    OP_FORLOOP,       ///< 数值for循环
    OP_FORPREP,       ///< for循环准备
    OP_TFORLOOP,      ///< 泛型for循环
    OP_SETLIST,       ///< 设置列表
    OP_CLOSE,         ///< 关闭上值
    OP_CLOSURE,       ///< 创建闭包
    OP_VARARG         ///< 可变参数
};

/**
 * @brief 指令结构
 */
struct Instruction {
    OpCode opcode;    ///< 操作码
    uint8_t a;        ///< 参数A
    uint16_t b;       ///< 参数B
    uint16_t c;       ///< 参数C
    
    /**
     * @brief 从32位整数构造指令
     * @param value 32位指令值
     */
    explicit Instruction(uint32_t value);
    
    /**
     * @brief 转换为32位整数
     * @return 32位指令值
     */
    uint32_t to_uint32() const;
    
    /**
     * @brief 获取Bx参数（B和C的组合）
     * @return Bx参数值
     */
    uint32_t get_bx() const;
    
    /**
     * @brief 获取sBx参数（有符号的Bx）
     * @return sBx参数值
     */
    int32_t get_sbx() const;
};

/* ========================================================================== */
/* 执行结果类型 */
/* ========================================================================== */

/**
 * @brief 执行结果状态
 */
enum class ExecutionStatus {
    SUCCESS,          ///< 执行成功
    ERROR,            ///< 执行错误
    YIELD,            ///< 协程让出
    BREAKPOINT        ///< 遇到断点
};

/**
 * @brief 指令执行结果
 */
struct InstructionResult {
    ExecutionStatus status;           ///< 执行状态
    std::optional<std::string> error; ///< 错误信息（如果有）
    bool should_jump;                 ///< 是否需要跳转
    int32_t jump_offset;              ///< 跳转偏移量
};

/**
 * @brief 虚拟机执行结果
 */
struct ExecutionResult {
    ExecutionStatus status;           ///< 执行状态
    std::optional<std::string> error; ///< 错误信息（如果有）
    std::vector<LuaValue> results;    ///< 返回值
    size_t instructions_executed;     ///< 执行的指令数
};

/* ========================================================================== */
/* 函数类型定义 */
/* ========================================================================== */

/**
 * @brief 指令处理器函数类型
 */
using InstructionHandler = std::function<InstructionResult(LuaState&, const Instruction&)>;

/**
 * @brief 调试钩子函数类型
 */
using DebugHook = std::function<void(LuaState&, const CallFrame&, const std::string& event)>;

} // namespace lua_vm

#endif /* VM_INTERFACE_H */