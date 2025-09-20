/**
 * @file vm-interface.hpp
 * @brief 虚拟机核心执行接口契约
 * @date 2025-09-20
 * @version 1.0.0
 * 
 * 定义Lua虚拟机的核心执行接口，包括指令分发、栈管理、函数调用等核心功能。
 * 此接口必须提供与Lua 5.1.5完全兼容的执行语义。
 */

#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <optional>
#include <string_view>

namespace lua::vm {

// 前置声明
class LuaValue;
class LuaFunction;
class LuaState;

/**
 * @brief 虚拟机指令类型
 * 
 * 完全兼容Lua 5.1.5的字节码指令集
 * 包括所有原版指令的功能和语义
 */
enum class Opcode : uint8_t {
    // 移动和加载指令
    OP_MOVE = 0,        // R(A) := R(B)
    OP_LOADK,           // R(A) := Kst(Bx)
    OP_LOADBOOL,        // R(A) := (Bool)B; if (C) pc++
    OP_LOADNIL,         // R(A) := ... := R(B) := nil
    OP_GETUPVAL,        // R(A) := UpValue[B]
    
    // 全局变量操作
    OP_GETGLOBAL,       // R(A) := Gbl[Kst(Bx)]
    OP_GETTABLE,        // R(A) := R(B)[RK(C)]
    OP_SETGLOBAL,       // Gbl[Kst(Bx)] := R(A)
    OP_SETUPVAL,        // UpValue[B] := R(A)
    OP_SETTABLE,        // R(A)[RK(B)] := RK(C)
    
    // 表操作
    OP_NEWTABLE,        // R(A) := {} (size = B,C)
    OP_SELF,            // R(A+1) := R(B); R(A) := R(B)[RK(C)]
    
    // 算术和逻辑运算
    OP_ADD,             // R(A) := RK(B) + RK(C)
    OP_SUB,             // R(A) := RK(B) - RK(C)
    OP_MUL,             // R(A) := RK(B) * RK(C)
    OP_DIV,             // R(A) := RK(B) / RK(C)
    OP_MOD,             // R(A) := RK(B) % RK(C)
    OP_POW,             // R(A) := RK(B) ^ RK(C)
    OP_UNM,             // R(A) := -R(B)
    OP_NOT,             // R(A) := not R(B)
    OP_LEN,             // R(A) := length of R(B)
    
    // 连接运算
    OP_CONCAT,          // R(A) := R(B).. ... ..R(C)
    
    // 跳转指令
    OP_JMP,             // pc+=sBx
    
    // 比较指令
    OP_EQ,              // if ((RK(B) == RK(C)) ~= A) then pc++
    OP_LT,              // if ((RK(B) <  RK(C)) ~= A) then pc++
    OP_LE,              // if ((RK(B) <= RK(C)) ~= A) then pc++
    
    // 测试指令
    OP_TEST,            // if not (R(A) <=> C) then pc++
    OP_TESTSET,         // if (R(B) <=> C) then R(A) := R(B) else pc++
    
    // 函数调用
    OP_CALL,            // R(A), ... ,R(A+C-2) := R(A)(R(A+1), ... ,R(A+B-1))
    OP_TAILCALL,        // return R(A)(R(A+1), ... ,R(A+B-1))
    OP_RETURN,          // return R(A), ... ,R(A+B-2)
    
    // 循环指令
    OP_FORLOOP,         // R(A)+=R(A+2); if R(A) <?= R(A+1) then { pc+=sBx; R(A+3)=R(A) }
    OP_FORPREP,         // R(A)-=R(A+2); pc+=sBx
    OP_TFORLOOP,        // R(A+3), ... ,R(A+2+C) := R(A)(R(A+1), R(A+2)); if R(A+3) ~= nil then R(A+2)=R(A+3) else pc++
    
    // 变参处理
    OP_SETLIST,         // R(A)[(C-1)*FPF+i] := R(A+i), 1 <= i <= B
    OP_CLOSE,           // close all variables in the stack up to (>=) R(A)
    OP_CLOSURE,         // R(A) := closure(KPROTO[Bx], R(A), ... ,R(A+n))
    OP_VARARG           // R(A), R(A+1), ..., R(A+B-1) = vararg
};

/**
 * @brief 指令结构
 * 
 * 兼容Lua 5.1.5的32位指令格式
 * 支持不同的指令格式：iABC, iABx, iAsBx
 */
struct Instruction {
    uint32_t value;
    
    // 指令格式解析
    Opcode opcode() const noexcept { return static_cast<Opcode>(value & 0x3F); }
    uint8_t arg_a() const noexcept { return (value >> 6) & 0xFF; }
    uint16_t arg_b() const noexcept { return (value >> 23) & 0x1FF; }
    uint16_t arg_c() const noexcept { return (value >> 14) & 0x1FF; }
    uint32_t arg_bx() const noexcept { return value >> 14; }
    int32_t arg_sbx() const noexcept { return static_cast<int32_t>(arg_bx()) - 131071; }
    
    // 指令构造
    static Instruction make_abc(Opcode op, uint8_t a, uint16_t b, uint16_t c) noexcept;
    static Instruction make_abx(Opcode op, uint8_t a, uint32_t bx) noexcept;
    static Instruction make_asbx(Opcode op, uint8_t a, int32_t sbx) noexcept;
};

/**
 * @brief 虚拟机执行器接口
 * 
 * 定义虚拟机的核心执行功能，包括指令分发、错误处理、调试支持等
 */
class IVMExecutor {
public:
    virtual ~IVMExecutor() = default;
    
    /**
     * @brief 执行字节码函数
     * @param state 虚拟机状态
     * @param function 要执行的函数
     * @param args 函数参数
     * @return 执行结果和返回值数量
     */
    virtual std::pair<bool, int> execute_function(
        LuaState* state, 
        LuaFunction* function,
        int args_count
    ) = 0;
    
    /**
     * @brief 单步执行指令
     * @param state 虚拟机状态
     * @param instruction 要执行的指令
     * @return 是否继续执行
     */
    virtual bool execute_instruction(LuaState* state, Instruction instruction) = 0;
    
    /**
     * @brief 处理函数调用
     * @param state 虚拟机状态
     * @param func_index 函数在栈中的位置
     * @param args_count 参数数量
     * @param results_count 期望返回值数量 (-1表示所有)
     * @return 实际返回值数量
     */
    virtual int handle_call(
        LuaState* state, 
        int func_index, 
        int args_count, 
        int results_count
    ) = 0;
    
    /**
     * @brief 处理函数返回
     * @param state 虚拟机状态
     * @param results_count 返回值数量
     * @return 是否成功返回
     */
    virtual bool handle_return(LuaState* state, int results_count) = 0;
    
    /**
     * @brief 处理运行时错误
     * @param state 虚拟机状态
     * @param error_message 错误消息
     */
    virtual void handle_error(LuaState* state, std::string_view error_message) = 0;
    
    /**
     * @brief 获取当前执行位置信息
     * @param state 虚拟机状态
     * @return 程序计数器值
     */
    virtual size_t get_program_counter(const LuaState* state) const = 0;
    
    /**
     * @brief 设置断点 (调试支持)
     * @param pc 程序计数器位置
     * @param enabled 是否启用断点
     */
    virtual void set_breakpoint(size_t pc, bool enabled) = 0;
    
    /**
     * @brief 获取执行统计信息
     * @return 指令执行次数统计
     */
    virtual std::vector<uint64_t> get_instruction_stats() const = 0;
};

/**
 * @brief 栈管理器接口
 * 
 * 管理Lua虚拟机的运行时栈，包括值栈和调用栈
 */
class IStackManager {
public:
    virtual ~IStackManager() = default;
    
    // 值栈操作
    virtual void push(const LuaValue& value) = 0;
    virtual LuaValue pop() = 0;
    virtual LuaValue& top(int index = -1) = 0;
    virtual const LuaValue& top(int index = -1) const = 0;
    virtual size_t size() const = 0;
    virtual void resize(size_t new_size) = 0;
    virtual void ensure_capacity(size_t capacity) = 0;
    
    // 索引访问 (兼容Lua API)
    virtual LuaValue& operator[](int index) = 0;
    virtual const LuaValue& operator[](int index) const = 0;
    
    // 调用栈操作
    virtual void push_call_frame(LuaFunction* func, size_t pc, size_t base, int args) = 0;
    virtual void pop_call_frame() = 0;
    virtual size_t call_depth() const = 0;
    
    // 栈状态查询
    virtual bool is_valid_index(int index) const = 0;
    virtual size_t abs_index(int index) const = 0;
    virtual void set_top(int index) = 0;
};

/**
 * @brief 全局环境管理器接口
 * 
 * 管理Lua的全局变量和环境表
 */
class IGlobalEnvironment {
public:
    virtual ~IGlobalEnvironment() = default;
    
    /**
     * @brief 获取全局变量
     * @param name 变量名
     * @return 变量值
     */
    virtual LuaValue get_global(std::string_view name) const = 0;
    
    /**
     * @brief 设置全局变量
     * @param name 变量名
     * @param value 变量值
     */
    virtual void set_global(std::string_view name, const LuaValue& value) = 0;
    
    /**
     * @brief 获取全局环境表
     * @return 全局环境表指针
     */
    virtual LuaValue get_globals_table() const = 0;
    
    /**
     * @brief 设置全局环境表
     * @param table 新的全局环境表
     */
    virtual void set_globals_table(const LuaValue& table) = 0;
};

} // namespace lua::vm

/**
 * @brief 契约验证宏
 * 
 * 用于在调试模式下验证接口契约的正确性
 */
#ifdef LUA_DEBUG_CONTRACTS
#define LUA_CONTRACT_REQUIRE(condition) assert(condition)
#define LUA_CONTRACT_ENSURE(condition) assert(condition)
#else
#define LUA_CONTRACT_REQUIRE(condition) ((void)0)
#define LUA_CONTRACT_ENSURE(condition) ((void)0)
#endif

/**
 * @brief 性能约束常量
 * 
 * 定义虚拟机性能相关的约束和限制
 */
namespace lua::vm::constraints {
    constexpr size_t MAX_STACK_SIZE = 1000000;           // 最大栈大小
    constexpr size_t MAX_CALL_DEPTH = 200;               // 最大调用深度
    constexpr size_t DEFAULT_STACK_SIZE = 20;            // 默认栈大小
    constexpr size_t INSTRUCTION_CACHE_SIZE = 1024;      // 指令缓存大小
}

/**
 * @brief 兼容性保证
 * 
 * 此接口保证与Lua 5.1.5的完全兼容性：
 * - 所有指令的语义与原版相同
 * - 栈操作行为完全一致
 * - 错误处理机制兼容
 * - 调用约定兼容
 */