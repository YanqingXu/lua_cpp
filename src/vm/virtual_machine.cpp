/**
 * @file virtual_machine.cpp
 * @brief Lua虚拟机核心实现
 * @description 实现Lua 5.1.5虚拟机的指令执行引擎和函数调用机制
 * @author Lua C++ Project
 * @date 2025-09-21
 */

#include "virtual_machine.h"
#include "../compiler/bytecode.h"
#include "../types/value.h"
#include "../core/lua_errors.h"
#include <chrono>
#include <algorithm>
#include <cassert>

namespace lua_cpp {

/* ========================================================================== */
/* 虚拟机构造和初始化 */
/* ========================================================================== */

VirtualMachine::VirtualMachine(const VMConfig& config)
    : config_(config)
    , stack_(std::make_unique<LuaStack>(config.stack_size, config.max_stack_size))
    , call_stack_()
    , execution_state_(ExecutionState::Stopped)
    , instruction_pointer_(0)
    , current_proto_(nullptr)
    , global_table_(nullptr)
    , statistics_()
    , debug_hook_(nullptr)
    , instruction_count_(0) {
    
    // 预分配调用栈
    call_stack_.reserve(config.max_call_depth);
    
    // 初始化全局表
    // global_table_ = std::make_shared<LuaTable>();
    
    Reset();
}

/* ========================================================================== */
/* 程序执行接口 */
/* ========================================================================== */

std::vector<LuaValue> VirtualMachine::ExecuteProgram(const Proto* proto, 
                                                     const std::vector<LuaValue>& args) {
    if (!proto) {
        throw VMExecutionError("Cannot execute null proto");
    }
    
    // 重置虚拟机状态
    Reset();
    
    // 设置主函数
    current_proto_ = proto;
    
    // 创建主函数调用帧
    call_stack_.emplace_back(proto, 0, args.size(), 0);
    
    // 将参数推入栈
    for (const auto& arg : args) {
        stack_->Push(arg);
    }
    
    // 开始执行
    execution_state_ = ExecutionState::Running;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    try {
        // 执行字节码直到完成
        ContinueExecution();
        
        // 记录执行时间
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double>(end_time - start_time);
        statistics_.execution_time = duration.count();
        
        // 收集返回值
        std::vector<LuaValue> results;
        while (!stack_->IsEmpty() && stack_->Size() > 0) {
            results.push_back(stack_->Pop());
        }
        
        // 反转结果（因为是从栈顶弹出的）
        std::reverse(results.begin(), results.end());
        
        return results;
        
    } catch (const LuaError& e) {
        execution_state_ = ExecutionState::Error;
        throw;
    }
}

void VirtualMachine::ExecuteInstruction(Instruction instruction) {
    if (execution_state_ != ExecutionState::Running) {
        throw VMExecutionError("VM is not in running state");
    }
    
    // 检查指令限制
    if (config_.enable_instruction_limit && 
        instruction_count_ >= config_.instruction_limit) {
        throw VMExecutionError("Instruction limit exceeded");
    }
    
    // 更新统计信息
    instruction_count_++;
    statistics_.total_instructions++;
    
    // 解码指令
    OpCode opcode = DecodeOpCode(instruction);
    RegisterIndex a = DecodeA(instruction);
    int b = DecodeB(instruction);
    int c = DecodeC(instruction);
    int bx = DecodeBx(instruction);
    int sbx = DecodeSBx(instruction);
    
    // 更新指令统计
    statistics_.instruction_counts[static_cast<int>(opcode)]++;
    
    // 调试钩子
    if (debug_hook_ && config_.enable_debug) {
        DebugInfo debug_info;
        debug_info.instruction_pointer = instruction_pointer_;
        debug_info.current_function = current_proto_;
        debug_info.current_opcode = opcode;
        debug_info.current_instruction = instruction;
        debug_info.current_line = GetCurrentLine();
        debug_hook_(debug_info);
    }
    
    // 执行指令
    switch (opcode) {
        case OpCode::MOVE:
            ExecuteMOVE(a, b);
            break;
            
        case OpCode::LOADK:
            ExecuteLOADK(a, bx);
            break;
            
        case OpCode::LOADBOOL:
            ExecuteLOADBOOL(a, b, c);
            break;
            
        case OpCode::LOADNIL:
            ExecuteLOADNIL(a, b);
            break;
            
        case OpCode::GETUPVAL:
            ExecuteGETUPVAL(a, b);
            break;
            
        case OpCode::GETGLOBAL:
            ExecuteGETGLOBAL(a, bx);
            break;
            
        case OpCode::GETTABLE:
            ExecuteGETTABLE(a, b, c);
            break;
            
        case OpCode::SETGLOBAL:
            ExecuteSETGLOBAL(a, bx);
            break;
            
        case OpCode::SETUPVAL:
            ExecuteSETUPVAL(a, b);
            break;
            
        case OpCode::SETTABLE:
            ExecuteSETTABLE(a, b, c);
            break;
            
        case OpCode::NEWTABLE:
            ExecuteNEWTABLE(a, b, c);
            break;
            
        case OpCode::SELF:
            ExecuteSELF(a, b, c);
            break;
            
        case OpCode::ADD:
            ExecuteADD(a, b, c);
            break;
            
        case OpCode::SUB:
            ExecuteSUB(a, b, c);
            break;
            
        case OpCode::MUL:
            ExecuteMUL(a, b, c);
            break;
            
        case OpCode::DIV:
            ExecuteDIV(a, b, c);
            break;
            
        case OpCode::MOD:
            ExecuteMOD(a, b, c);
            break;
            
        case OpCode::POW:
            ExecutePOW(a, b, c);
            break;
            
        case OpCode::UNM:
            ExecuteUNM(a, b);
            break;
            
        case OpCode::NOT:
            ExecuteNOT(a, b);
            break;
            
        case OpCode::LEN:
            ExecuteLEN(a, b);
            break;
            
        case OpCode::CONCAT:
            ExecuteCONCAT(a, b, c);
            break;
            
        case OpCode::JMP:
            ExecuteJMP(sbx);
            break;
            
        case OpCode::EQ:
            ExecuteEQ(a, b, c);
            break;
            
        case OpCode::LT:
            ExecuteLT(a, b, c);
            break;
            
        case OpCode::LE:
            ExecuteLE(a, b, c);
            break;
            
        case OpCode::TEST:
            ExecuteTEST(a, c);
            break;
            
        case OpCode::TESTSET:
            ExecuteTESTSET(a, b, c);
            break;
            
        case OpCode::CALL:
            ExecuteCALL(a, b, c);
            break;
            
        case OpCode::TAILCALL:
            ExecuteTAILCALL(a, b, c);
            break;
            
        case OpCode::RETURN:
            ExecuteRETURN(a, b);
            break;
            
        case OpCode::FORLOOP:
            ExecuteFORLOOP(a, sbx);
            break;
            
        case OpCode::FORPREP:
            ExecuteFORPREP(a, sbx);
            break;
            
        case OpCode::TFORLOOP:
            ExecuteTFORLOOP(a, c);
            break;
            
        case OpCode::SETLIST:
            ExecuteSETLIST(a, b, c);
            break;
            
        case OpCode::CLOSE:
            ExecuteCLOSE(a);
            break;
            
        case OpCode::CLOSURE:
            ExecuteCLOSURE(a, bx);
            break;
            
        case OpCode::VARARG:
            ExecuteVARARG(a, b);
            break;
            
        default:
            throw InvalidInstructionError("Unknown opcode: " + std::to_string(static_cast<int>(opcode)));
    }
    
    // 递增指令指针（除非被跳转指令修改）
    if (opcode != OpCode::JMP && opcode != OpCode::FORLOOP && 
        opcode != OpCode::FORPREP && opcode != OpCode::RETURN) {
        instruction_pointer_++;
    }
}

Size VirtualMachine::ExecuteInstructions(Size max_instructions) {
    Size executed = 0;
    
    while (execution_state_ == ExecutionState::Running && 
           (max_instructions == 0 || executed < max_instructions)) {
        
        if (!HasMoreInstructions()) {
            execution_state_ = ExecutionState::Completed;
            break;
        }
        
        Instruction instruction = GetNextInstruction();
        ExecuteInstruction(instruction);
        executed++;
    }
    
    return executed;
}

bool VirtualMachine::StepExecution() {
    if (execution_state_ != ExecutionState::Running) {
        return false;
    }
    
    if (!HasMoreInstructions()) {
        execution_state_ = ExecutionState::Completed;
        return false;
    }
    
    Instruction instruction = GetNextInstruction();
    ExecuteInstruction(instruction);
    
    return HasMoreInstructions();
}

void VirtualMachine::ContinueExecution() {
    while (execution_state_ == ExecutionState::Running) {
        if (!StepExecution()) {
            break;
        }
    }
}

void VirtualMachine::Suspend() {
    execution_state_ = ExecutionState::Suspended;
}

void VirtualMachine::Reset() {
    execution_state_ = ExecutionState::Stopped;
    instruction_pointer_ = 0;
    current_proto_ = nullptr;
    call_stack_.clear();
    stack_->Clear();
    instruction_count_ = 0;
    
    // 重置统计信息
    statistics_ = ExecutionStatistics();
}

/* ========================================================================== */
/* 状态管理 */
/* ========================================================================== */

Size VirtualMachine::GetInstructionPointer() const {
    return instruction_pointer_;
}

void VirtualMachine::SetInstructionPointer(Size ip) {
    instruction_pointer_ = ip;
}

bool VirtualMachine::HasMoreInstructions() const {
    if (!current_proto_ || call_stack_.empty()) {
        return false;
    }
    
    return instruction_pointer_ < current_proto_->instructions.size();
}

Instruction VirtualMachine::GetNextInstruction() const {
    if (!HasMoreInstructions()) {
        throw VMExecutionError("No more instructions to execute");
    }
    
    return current_proto_->instructions[instruction_pointer_];
}

int VirtualMachine::GetCurrentLine() const {
    if (!current_proto_ || instruction_pointer_ >= current_proto_->line_info.size()) {
        return 0;
    }
    
    return current_proto_->line_info[instruction_pointer_];
}

/* ========================================================================== */
/* 堆栈管理 */
/* ========================================================================== */

void VirtualMachine::SetRegister(RegisterIndex reg, const LuaValue& value) {
    Size stack_index = GetCurrentBase() + reg;
    
    // 确保栈有足够空间
    while (stack_->Size() <= stack_index) {
        stack_->Push(LuaValue());
    }
    
    stack_->Set(stack_index, value);
}

LuaValue VirtualMachine::GetRegister(RegisterIndex reg) const {
    Size stack_index = GetCurrentBase() + reg;
    
    if (stack_index >= stack_->Size()) {
        return LuaValue(); // 返回nil
    }
    
    return stack_->Get(stack_index);
}

LuaValue VirtualMachine::GetRK(int rk) const {
    if (rk & 0x100) {
        // 常量
        int const_index = rk & 0xFF;
        if (!current_proto_ || const_index >= current_proto_->constants.size()) {
            throw VMExecutionError("Invalid constant index");
        }
        return current_proto_->constants[const_index];
    } else {
        // 寄存器
        return GetRegister(static_cast<RegisterIndex>(rk));
    }
}

Size VirtualMachine::GetCurrentBase() const {
    if (call_stack_.empty()) {
        return 0;
    }
    
    return call_stack_.back().GetBase();
}

/* ========================================================================== */
/* 函数调用管理 */
/* ========================================================================== */

void VirtualMachine::PushCallFrame(const Proto* proto, Size base, Size param_count) {
    if (call_stack_.size() >= config_.max_call_depth) {
        throw CallStackOverflowError("Maximum call depth exceeded");
    }
    
    // 创建新的调用帧
    call_stack_.emplace_back(proto, base, param_count, instruction_pointer_ + 1);
    
    // 更新当前函数和指令指针
    current_proto_ = proto;
    instruction_pointer_ = 0;
    
    // 更新统计信息
    statistics_.function_calls++;
    statistics_.peak_call_depth = std::max(statistics_.peak_call_depth, call_stack_.size());
}

void VirtualMachine::PopCallFrame() {
    if (call_stack_.empty()) {
        throw CallFrameError("Cannot pop from empty call stack");
    }
    
    // 恢复返回地址
    Size return_address = call_stack_.back().GetReturnAddress();
    
    // 弹出调用帧
    call_stack_.pop_back();
    
    if (call_stack_.empty()) {
        // 主函数返回，程序结束
        execution_state_ = ExecutionState::Completed;
        current_proto_ = nullptr;
        instruction_pointer_ = 0;
    } else {
        // 恢复上一层函数的执行上下文
        current_proto_ = call_stack_.back().GetProto();
        instruction_pointer_ = return_address;
    }
}

/* ========================================================================== */
/* 指令解码辅助函数 */
/* ========================================================================== */

OpCode VirtualMachine::DecodeOpCode(Instruction inst) const {
    return static_cast<OpCode>(inst & 0x3F);
}

RegisterIndex VirtualMachine::DecodeA(Instruction inst) const {
    return static_cast<RegisterIndex>((inst >> 6) & 0xFF);
}

int VirtualMachine::DecodeB(Instruction inst) const {
    return static_cast<int>((inst >> 14) & 0x1FF);
}

int VirtualMachine::DecodeC(Instruction inst) const {
    return static_cast<int>((inst >> 23) & 0x1FF);
}

int VirtualMachine::DecodeBx(Instruction inst) const {
    return static_cast<int>((inst >> 14) & 0x3FFFF);
}

int VirtualMachine::DecodeSBx(Instruction inst) const {
    constexpr int MAXARG_sBx = 0x20000 - 1; // (1 << 18) / 2 - 1
    return DecodeBx(inst) - MAXARG_sBx;
}

} // namespace lua_cpp