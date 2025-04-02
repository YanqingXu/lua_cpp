#pragma once

#include "types.hpp"
#include "instruction.hpp"
#include "state.hpp"
#include "callinfo.hpp"
#include "function_proto.hpp"
#include "object/function.hpp"

namespace Lua {

/**
 * @brief Lua虚拟机执行引擎
 *
 * 虚拟机负责执行编译后的Lua字节码。它维护程序计数器(PC)，
 * 管理调用栈和执行指令。
 */
class VM {
public:
    explicit VM(Ptr<State> state);
    ~VM() = default;

    // 禁止拷贝
    VM(const VM&) = delete;
    VM& operator=(const VM&) = delete;

    // 主要执行函数
    i32 execute(Ptr<Function> function, i32 nargs, i32 nresults);

private:
    // 内部执行一条指令
    void executeInstruction(const Instruction& instr);

    // 处理各种类型的操作码
    void executeArithmetic(const Instruction& instr);
    void executeComparison(const Instruction& instr);
    void executeLoadConstant(const Instruction& instr);
    void executeJump(const Instruction& instr);
    void executeCall(const Instruction& instr);
    void executeReturn(const Instruction& instr);
    void executeTableOperations(const Instruction& instr);
    void executeUpvalueOperations(const Instruction& instr);
    void executeOtherOperations(const Instruction& instr);

    // 获取当前正在执行的函数/原型
    Ptr<Function> getCurrentFunction() const;
    Ptr<FunctionProto> getCurrentProto() const;

    // 访问寄存器和常量
    Value& getRegister(i32 reg);
    Value getConstant(i32 idx) const;

    // 调用栈管理
    CallInfo& pushCallInfo(Ptr<Function> func, i32 nargs, i32 returnPC);
    void popCallInfo();

    // 成员变量
    Ptr<State> m_state;         // 关联的Lua状态
    Vec<CallInfo> m_callStack;      // 调用栈
    i32 m_pc;                               // 程序计数器
    bool m_running;                         // 是否正在运行
};

} // namespace Lua
