/**
 * @file virtual_machine.cpp
 * @brief Lua虚拟机核心实现
 * @description 实现Lua 5.1.5虚拟机的指令执行引擎和函数调用机制
 * @author Lua C++ Project
 * @date 2025-09-26
 * @version T025 - Complete VM Implementation
 */

#include "virtual_machine.h"
#include "../compiler/bytecode.h"
#include "../core/lua_common.h"
#include "../core/lua_errors.h"
#include "../types/value.h"
#include "../types/lua_table.h"
#include "../memory/garbage_collector.h"
#include <chrono>
#include <algorithm>
#include <cassert>
#include <iostream>
#include <sstream>

namespace lua_cpp {

/* ========================================================================== */
/* 虚拟机构造和初始化 */
/* ========================================================================== */

VirtualMachine::VirtualMachine(const VMConfig& config)
    : config_(config)
    , stack_(std::make_unique<LuaStack>(config.initial_stack_size))
    , call_stack_(std::make_unique<CallStack>(config.max_call_depth))
    , execution_state_(ExecutionState::Ready)
    , instruction_pointer_(0)
    , current_proto_(nullptr)
    , global_table_(std::make_shared<LuaTable>())
    , debug_hook_(nullptr)
    , statistics_()
    , instruction_count_(0) {
    
    // 初始化统计信息
    statistics_ = ExecutionStatistics{};
    
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
    PushCallFrame(proto, 0, args.size(), 0);
    
    // 将参数推入栈
    for (const auto& arg : args) {
        Push(arg);
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
        Size top = GetStackTop();
        for (Size i = 0; i < top; ++i) {
            results.push_back(GetStack(i));
        }
        
        return results;
        
    } catch (const LuaError& e) {
        execution_state_ = ExecutionState::Error;
        throw;
    }
}

void VirtualMachine::ExecuteInstruction(Instruction instruction) {
    if (execution_state_ != ExecutionState::Running) {
        throw VMExecutionError("VM is not in running state: " + 
                              std::to_string(static_cast<int>(execution_state_)));
    }
    
    // 检查指令限制
    if (config_.enable_instruction_limit && 
        instruction_count_ >= config_.instruction_limit) {
        throw VMExecutionError("Instruction limit exceeded: " + 
                              std::to_string(instruction_count_) + "/" + 
                              std::to_string(config_.instruction_limit));
    }
    
    // 检查调用栈完整性
    if (call_stack_->IsEmpty()) {
        throw VMExecutionError("No active call frame");
    }
    
    // 更新统计信息
    instruction_count_++;
    statistics_.total_instructions++;
    
    // 解码指令
    OpCode opcode = GetOpCode(instruction);
    RegisterIndex a = GetArgA(instruction);
    int b = GetArgB(instruction);
    int c = GetArgC(instruction);
    int bx = GetArgBx(instruction);
    int sbx = GetArgsBx(instruction);
    
    // 验证操作码
    if (static_cast<int>(opcode) >= static_cast<int>(OpCode::NUM_OPCODES)) {
        throw InvalidInstructionError("Invalid opcode: " + std::to_string(static_cast<int>(opcode)));
    }
    
    // 更新指令统计
    statistics_.instruction_counts[static_cast<int>(opcode)]++;
    
    // 调试钩子
    if (debug_hook_ && config_.enable_debug_info) {
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
            execution_state_ = ExecutionState::Finished;
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
        execution_state_ = ExecutionState::Finished;
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
    execution_state_ = ExecutionState::Ready;
    instruction_pointer_ = 0;
    current_proto_ = nullptr;
    call_stack_->Clear();
    SetStackTop(0);
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
    if (!current_proto_ || call_stack_->IsEmpty()) {
        return false;
    }
    
    return instruction_pointer_ < current_proto_->GetCodeSize();
}

Instruction VirtualMachine::GetNextInstruction() const {
    if (!HasMoreInstructions()) {
        throw VMExecutionError("No more instructions to execute");
    }
    
    return current_proto_->GetInstruction(instruction_pointer_);
}

int VirtualMachine::GetCurrentLine() const {
    if (!current_proto_) {
        return 0;
    }
    
    // 简化版本：返回指令指针作为行号
    return static_cast<int>(instruction_pointer_) + 1;
}

/* ========================================================================== */
/* 堆栈管理 */
/* ========================================================================== */

void VirtualMachine::SetRegister(RegisterIndex reg, const LuaValue& value) {
    Size stack_index = GetCurrentBase() + reg;
    
    // 检查寄存器索引合法性
    if (reg > 255) { // Lua寄存器最大值
        throw VMExecutionError("Register index out of range: " + std::to_string(reg));
    }
    
    // 检查栈大小限制
    Size required_size = stack_index + 1;
    if (required_size > GetMaxStackSize()) {
        throw VMExecutionError("Stack overflow: required " + std::to_string(required_size) + 
                              ", max " + std::to_string(GetMaxStackSize()));
    }
    
    // 确保栈有足够空间
    while (GetStackTop() <= stack_index) {
        Push(LuaValue());
    }
    
    SetStack(stack_index, value);
    
    // 更新峰值堆栈使用量
    statistics_.peak_stack_usage = std::max(statistics_.peak_stack_usage, GetStackTop());
}

LuaValue VirtualMachine::GetRegister(RegisterIndex reg) const {
    // 检查寄存器索引合法性
    if (reg > 255) { // Lua寄存器最大值
        throw VMExecutionError("Register index out of range: " + std::to_string(reg));
    }
    
    Size stack_index = GetCurrentBase() + reg;
    
    if (stack_index >= GetStackTop()) {
        return LuaValue(); // 返回nil
    }
    
    return GetStack(stack_index);
}

LuaValue VirtualMachine::GetRK(int rk) const {
    if (IsConstant(rk)) {
        // 常量
        int const_index = RKToConstantIndex(rk);
        if (!current_proto_ || const_index >= current_proto_->GetConstantCount()) {
            throw VMExecutionError("Invalid constant index");
        }
        return current_proto_->GetConstant(const_index);
    } else {
        // 寄存器
        return GetRegister(static_cast<RegisterIndex>(rk));
    }
}

Size VirtualMachine::GetCurrentBase() const {
    if (call_stack_->IsEmpty()) {
        return 0;
    }
    
    return call_stack_->GetCurrentFrame().GetBase();
}

/* ========================================================================== */
/* 函数调用管理 */
/* ========================================================================== */

void VirtualMachine::PushCallFrame(const Proto* proto, Size base, Size param_count) {
    // 创建新的调用帧
    call_stack_->PushFrame(proto, base, param_count, instruction_pointer_ + 1);
    
    // 更新当前函数和指令指针
    current_proto_ = proto;
    instruction_pointer_ = 0;
    
    // 更新统计信息
    statistics_.function_calls++;
    statistics_.peak_call_depth = std::max(statistics_.peak_call_depth, call_stack_->GetDepth());
}

void VirtualMachine::PopCallFrame() {
    // 获取返回地址
    Size return_address = call_stack_->GetCurrentFrame().GetReturnAddress();
    
    // 弹出调用帧
    call_stack_->PopFrame();
    
    if (call_stack_->IsEmpty()) {
        // 主函数返回，程序结束
        execution_state_ = ExecutionState::Finished;
        current_proto_ = nullptr;
        instruction_pointer_ = 0;
    } else {
        // 恢复上一层函数的执行上下文
        current_proto_ = call_stack_->GetCurrentFrame().GetProto();
        instruction_pointer_ = return_address;
    }
}

/* ========================================================================== */
/* 指令解码辅助函数 */
/* ========================================================================== */

OpCode VirtualMachine::DecodeOpCode(Instruction inst) const {
    return GetOpCode(inst);
}

RegisterIndex VirtualMachine::DecodeA(Instruction inst) const {
    return static_cast<RegisterIndex>(GetArgA(inst));
}

int VirtualMachine::DecodeB(Instruction inst) const {
    return GetArgB(inst);
}

int VirtualMachine::DecodeC(Instruction inst) const {
    return GetArgC(inst);
}

int VirtualMachine::DecodeBx(Instruction inst) const {
    return GetArgBx(inst);
}

int VirtualMachine::DecodeSBx(Instruction inst) const {
    return GetArgsBx(inst);
}

/* ========================================================================== */
/* 统计和诊断方法 */
/* ========================================================================== */

void VirtualMachine::ResetStatistics() {
    statistics_ = ExecutionStatistics{};
}

Size VirtualMachine::GetMemoryUsage() const {
    // 简化版本：返回堆栈大小
    return GetStackSize() * sizeof(LuaValue);
}

DebugInfo VirtualMachine::GetCurrentDebugInfo() const {
    DebugInfo info;
    info.instruction_pointer = instruction_pointer_;
    info.current_function = current_proto_;
    
    if (!call_stack_->IsEmpty()) {
        Instruction inst = GetNextInstruction();
        info.current_opcode = GetOpCode(inst);
        info.current_instruction = inst;
    } else {
        info.current_opcode = OpCode::MOVE; // 默认值
        info.current_instruction = 0;
    }
    
    info.current_line = GetCurrentLine();
    info.source_name = current_proto_ ? current_proto_->GetSourceName() : "";
    info.function_name = ""; // 暂时为空
    
    return info;
}

std::string VirtualMachine::GetStackTrace() const {
    if (call_stack_->IsEmpty()) {
        return "Empty call stack";
    }
    
    return call_stack_->FormatStackTrace();
}

/* ========================================================================== */
/* 工厂函数 */
/* ========================================================================== */

std::unique_ptr<VirtualMachine> CreateStandardVM() {
    VMConfig config;
    config.initial_stack_size = VM_DEFAULT_STACK_SIZE;
    config.max_stack_size = VM_MAX_STACK_SIZE;
    config.max_call_depth = VM_MAX_CALL_STACK_DEPTH;
    config.enable_debug_info = false;
    config.enable_profiling = false;
    config.enable_stack_trace = true;
    
    return std::make_unique<VirtualMachine>(config);
}

std::unique_ptr<VirtualMachine> CreateDebugVM() {
    VMConfig config;
    config.initial_stack_size = VM_DEFAULT_STACK_SIZE;
    config.max_stack_size = VM_MAX_STACK_SIZE;
    config.max_call_depth = VM_MAX_CALL_STACK_DEPTH;
    config.enable_debug_info = true;
    config.enable_profiling = true;
    config.enable_stack_trace = true;
    
    return std::make_unique<VirtualMachine>(config);
}

std::unique_ptr<VirtualMachine> CreateHighPerformanceVM() {
    VMConfig config;
    config.initial_stack_size = VM_DEFAULT_STACK_SIZE * 2;
    config.max_stack_size = VM_MAX_STACK_SIZE * 2;
    config.max_call_depth = VM_MAX_CALL_STACK_DEPTH * 2;
    config.enable_debug_info = false;
    config.enable_profiling = false;
    config.enable_stack_trace = false;
    config.enable_instruction_limit = false;
    
    return std::make_unique<VirtualMachine>(config);
}

std::unique_ptr<VirtualMachine> CreateEmbeddedVM() {
    VMConfig config;
    config.initial_stack_size = 256;
    config.max_stack_size = 1024;
    config.max_call_depth = 50;
    config.enable_debug_info = false;
    config.enable_profiling = false;
    config.enable_stack_trace = false;
    config.enable_instruction_limit = true;
    config.instruction_limit = 10000;
    
    return std::make_unique<VirtualMachine>(config);
}

} // namespace lua_cpp