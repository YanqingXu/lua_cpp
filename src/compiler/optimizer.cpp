/**
 * @file optimizer.cpp
 * @brief 字节码优化器实现
 * @description 实现各种编译优化技术，包括常量折叠、死代码消除等
 * @author Lua C++ Project
 * @date 2025-09-21
 */

#include "optimizer.h"
#include <algorithm>
#include <unordered_set>

namespace lua_cpp {

/* ========================================================================== */
/* 字节码优化器实现 */
/* ========================================================================== */

BytecodeOptimizer::BytecodeOptimizer(const OptimizationConfig& config)
    : config_(config) {
}

void BytecodeOptimizer::Optimize(std::vector<Instruction>& instructions,
                                 std::vector<LuaValue>& constants,
                                 std::vector<int>& line_info) {
    if (config_.IsEnabled(OptimizationType::ConstantFolding)) {
        PerformConstantFolding(instructions, constants);
    }
    
    if (config_.IsEnabled(OptimizationType::DeadCodeElimination)) {
        PerformDeadCodeElimination(instructions, line_info);
    }
    
    if (config_.IsEnabled(OptimizationType::JumpOptimization)) {
        PerformJumpOptimization(instructions);
    }
    
    if (config_.IsEnabled(OptimizationType::LocalVariableReuse)) {
        PerformLocalVariableReuse(instructions);
    }
}

void BytecodeOptimizer::PerformConstantFolding(std::vector<Instruction>& instructions,
                                               std::vector<LuaValue>& constants) {
    for (Size i = 0; i < instructions.size(); ++i) {
        Instruction& inst = instructions[i];
        OpCode op = DecodeOpCode(inst);
        
        // 检查是否是可以常量折叠的二元运算
        if (IsFoldableBinaryOp(op)) {
            RegisterIndex a = DecodeA(inst);
            int b = DecodeB(inst);
            int c = DecodeC(inst);
            
            // 检查操作数是否都是常量
            if (IsConstantLoad(instructions, i, b) && IsConstantLoad(instructions, i, c)) {
                LuaValue const1 = GetConstantValue(instructions, constants, i, b);
                LuaValue const2 = GetConstantValue(instructions, constants, i, c);
                
                // 尝试计算常量结果
                LuaValue result;
                if (TryFoldConstants(op, const1, const2, result)) {
                    // 添加结果常量
                    int const_idx = AddConstant(constants, result);
                    
                    // 替换为 LOADK 指令
                    inst = EncodeABx(OpCode::LOADK, a, const_idx);
                    
                    // 标记前面的常量加载指令为可删除
                    MarkForDeletion(instructions, i, b);
                    MarkForDeletion(instructions, i, c);
                }
            }
        }
        // 检查一元运算
        else if (IsFoldableUnaryOp(op)) {
            RegisterIndex a = DecodeA(inst);
            int b = DecodeB(inst);
            
            if (IsConstantLoad(instructions, i, b)) {
                LuaValue const_val = GetConstantValue(instructions, constants, i, b);
                
                LuaValue result;
                if (TryFoldUnaryConstant(op, const_val, result)) {
                    int const_idx = AddConstant(constants, result);
                    inst = EncodeABx(OpCode::LOADK, a, const_idx);
                    MarkForDeletion(instructions, i, b);
                }
            }
        }
    }
    
    // 删除标记的指令
    RemoveMarkedInstructions(instructions);
}

void BytecodeOptimizer::PerformDeadCodeElimination(std::vector<Instruction>& instructions,
                                                    std::vector<int>& line_info) {
    std::vector<bool> reachable(instructions.size(), false);
    std::vector<Size> worklist;
    
    // 标记入口点为可达
    if (!instructions.empty()) {
        reachable[0] = true;
        worklist.push_back(0);
    }
    
    // 传播可达性
    while (!worklist.empty()) {
        Size pc = worklist.back();
        worklist.pop_back();
        
        if (pc >= instructions.size()) continue;
        
        Instruction inst = instructions[pc];
        OpCode op = DecodeOpCode(inst);
        
        // 处理分支指令
        switch (op) {
            case OpCode::JMP: {
                int sbx = DecodeSBx(inst);
                Size target = pc + 1 + sbx;
                if (target < instructions.size() && !reachable[target]) {
                    reachable[target] = true;
                    worklist.push_back(target);
                }
                break;
            }
            case OpCode::TEST:
            case OpCode::TESTSET: {
                // 条件跳转：下一条指令应该是跳转
                if (pc + 1 < instructions.size()) {
                    if (!reachable[pc + 1]) {
                        reachable[pc + 1] = true;
                        worklist.push_back(pc + 1);
                    }
                }
                if (pc + 2 < instructions.size()) {
                    if (!reachable[pc + 2]) {
                        reachable[pc + 2] = true;
                        worklist.push_back(pc + 2);
                    }
                }
                break;
            }
            case OpCode::RETURN:
                // 返回指令后的代码不可达
                break;
            default:
                // 普通指令：下一条指令可达
                if (pc + 1 < instructions.size() && !reachable[pc + 1]) {
                    reachable[pc + 1] = true;
                    worklist.push_back(pc + 1);
                }
                break;
        }
    }
    
    // 移除不可达指令
    std::vector<Instruction> new_instructions;
    std::vector<int> new_line_info;
    
    for (Size i = 0; i < instructions.size(); ++i) {
        if (reachable[i]) {
            new_instructions.push_back(instructions[i]);
            if (i < line_info.size()) {
                new_line_info.push_back(line_info[i]);
            }
        }
    }
    
    instructions = std::move(new_instructions);
    line_info = std::move(new_line_info);
}

void BytecodeOptimizer::PerformJumpOptimization(std::vector<Instruction>& instructions) {
    bool changed = true;
    
    while (changed) {
        changed = false;
        
        for (Size i = 0; i < instructions.size(); ++i) {
            Instruction& inst = instructions[i];
            OpCode op = DecodeOpCode(inst);
            
            if (op == OpCode::JMP) {
                int sbx = DecodeSBx(inst);
                Size target = i + 1 + sbx;
                
                // 跳转到跳转优化 (jump to jump)
                if (target < instructions.size()) {
                    Instruction target_inst = instructions[target];
                    OpCode target_op = DecodeOpCode(target_inst);
                    
                    if (target_op == OpCode::JMP) {
                        int target_sbx = DecodeSBx(target_inst);
                        Size final_target = target + 1 + target_sbx;
                        
                        // 更新跳转目标
                        int new_offset = static_cast<int>(final_target) - static_cast<int>(i + 1);
                        inst = EncodeAsBx(OpCode::JMP, 0, new_offset);
                        changed = true;
                    }
                }
                
                // 删除跳转到下一条指令的跳转
                if (sbx == 0) {
                    inst = EncodeABC(OpCode::MOVE, 0, 0, 0); // NOP
                    changed = true;
                }
            }
        }
    }
}

void BytecodeOptimizer::PerformLocalVariableReuse(std::vector<Instruction>& instructions) {
    std::vector<bool> register_used(256, false); // 假设最多256个寄存器
    std::vector<Size> last_use(256, SIZE_MAX);
    
    // 分析寄存器使用情况
    for (Size i = 0; i < instructions.size(); ++i) {
        Instruction inst = instructions[i];
        OpCode op = DecodeOpCode(inst);
        
        // 记录寄存器使用
        RegisterIndex a = DecodeA(inst);
        int b = DecodeB(inst);
        int c = DecodeC(inst);
        
        switch (GetInstructionMode(op)) {
            case InstructionMode::iABC:
                register_used[a] = true;
                last_use[a] = i;
                if (b < 256) {
                    register_used[b] = true;
                    last_use[b] = i;
                }
                if (c < 256) {
                    register_used[c] = true;
                    last_use[c] = i;
                }
                break;
            case InstructionMode::iABx:
            case InstructionMode::iAsBx:
                register_used[a] = true;
                last_use[a] = i;
                break;
        }
    }
    
    // TODO: 实现更高级的寄存器重用算法
    // 这里可以实现活跃变量分析和寄存器重分配
}

/* ========================================================================== */
/* 辅助方法实现 */
/* ========================================================================== */

bool BytecodeOptimizer::IsFoldableBinaryOp(OpCode op) const {
    switch (op) {
        case OpCode::ADD:
        case OpCode::SUB:
        case OpCode::MUL:
        case OpCode::DIV:
        case OpCode::MOD:
        case OpCode::POW:
            return true;
        default:
            return false;
    }
}

bool BytecodeOptimizer::IsFoldableUnaryOp(OpCode op) const {
    switch (op) {
        case OpCode::UNM:
        case OpCode::NOT:
        case OpCode::LEN:
            return true;
        default:
            return false;
    }
}

bool BytecodeOptimizer::IsConstantLoad(const std::vector<Instruction>& instructions,
                                       Size current_pc, int reg) const {
    // 向后查找最近的对该寄存器的赋值
    for (int i = static_cast<int>(current_pc) - 1; i >= 0; --i) {
        Instruction inst = instructions[i];
        OpCode op = DecodeOpCode(inst);
        RegisterIndex a = DecodeA(inst);
        
        if (a == reg) {
            return op == OpCode::LOADK || op == OpCode::LOADBOOL || op == OpCode::LOADNIL;
        }
    }
    
    return false;
}

LuaValue BytecodeOptimizer::GetConstantValue(const std::vector<Instruction>& instructions,
                                             const std::vector<LuaValue>& constants,
                                             Size current_pc, int reg) const {
    // 向后查找最近的对该寄存器的赋值
    for (int i = static_cast<int>(current_pc) - 1; i >= 0; --i) {
        Instruction inst = instructions[i];
        OpCode op = DecodeOpCode(inst);
        RegisterIndex a = DecodeA(inst);
        
        if (a == reg) {
            switch (op) {
                case OpCode::LOADK: {
                    int bx = DecodeBx(inst);
                    if (bx < constants.size()) {
                        return constants[bx];
                    }
                    break;
                }
                case OpCode::LOADBOOL: {
                    int b = DecodeB(inst);
                    return LuaValue(b != 0);
                }
                case OpCode::LOADNIL:
                    return LuaValue();
            }
        }
    }
    
    return LuaValue(); // 返回nil作为默认值
}

bool BytecodeOptimizer::TryFoldConstants(OpCode op, const LuaValue& left, 
                                         const LuaValue& right, LuaValue& result) const {
    // 只对数字进行折叠
    if (!left.IsNumber() || !right.IsNumber()) {
        return false;
    }
    
    double a = left.AsNumber();
    double b = right.AsNumber();
    
    switch (op) {
        case OpCode::ADD:
            result = LuaValue(a + b);
            return true;
        case OpCode::SUB:
            result = LuaValue(a - b);
            return true;
        case OpCode::MUL:
            result = LuaValue(a * b);
            return true;
        case OpCode::DIV:
            if (b != 0.0) {
                result = LuaValue(a / b);
                return true;
            }
            break;
        case OpCode::MOD:
            if (b != 0.0) {
                result = LuaValue(fmod(a, b));
                return true;
            }
            break;
        case OpCode::POW:
            result = LuaValue(pow(a, b));
            return true;
    }
    
    return false;
}

bool BytecodeOptimizer::TryFoldUnaryConstant(OpCode op, const LuaValue& operand, 
                                             LuaValue& result) const {
    switch (op) {
        case OpCode::UNM:
            if (operand.IsNumber()) {
                result = LuaValue(-operand.AsNumber());
                return true;
            }
            break;
        case OpCode::NOT:
            result = LuaValue(!operand.IsTruthy());
            return true;
        case OpCode::LEN:
            if (operand.IsString()) {
                result = LuaValue(static_cast<double>(operand.AsString().length()));
                return true;
            }
            break;
    }
    
    return false;
}

int BytecodeOptimizer::AddConstant(std::vector<LuaValue>& constants, const LuaValue& value) {
    // 查找是否已存在
    for (Size i = 0; i < constants.size(); ++i) {
        if (constants[i] == value) {
            return static_cast<int>(i);
        }
    }
    
    // 添加新常量
    constants.push_back(value);
    return static_cast<int>(constants.size() - 1);
}

void BytecodeOptimizer::MarkForDeletion(std::vector<Instruction>& instructions,
                                         Size current_pc, int reg) {
    // 向后查找对该寄存器的最近赋值并标记删除
    for (int i = static_cast<int>(current_pc) - 1; i >= 0; --i) {
        Instruction inst = instructions[i];
        RegisterIndex a = DecodeA(inst);
        
        if (a == reg) {
            // 标记为NOP（使用特殊的指令编码）
            instructions[i] = EncodeABC(OpCode::MOVE, 0, 0, 0);
            break;
        }
    }
}

void BytecodeOptimizer::RemoveMarkedInstructions(std::vector<Instruction>& instructions) {
    instructions.erase(
        std::remove_if(instructions.begin(), instructions.end(),
            [](Instruction inst) {
                // 检查是否是标记删除的NOP指令
                return DecodeOpCode(inst) == OpCode::MOVE && 
                       DecodeA(inst) == 0 && DecodeB(inst) == 0 && DecodeC(inst) == 0;
            }),
        instructions.end()
    );
}

InstructionMode BytecodeOptimizer::GetInstructionMode(OpCode op) const {
    // 根据操作码返回指令格式
    // 这里需要根据实际的Lua 5.1.5操作码定义来实现
    switch (op) {
        case OpCode::LOADK:
        case OpCode::GETGLOBAL:
        case OpCode::SETGLOBAL:
        case OpCode::CLOSURE:
            return InstructionMode::iABx;
        case OpCode::JMP:
        case OpCode::FORLOOP:
        case OpCode::FORPREP:
            return InstructionMode::iAsBx;
        default:
            return InstructionMode::iABC;
    }
}

// 指令解码辅助函数
OpCode BytecodeOptimizer::DecodeOpCode(Instruction inst) const {
    return static_cast<OpCode>(inst & 0x3F);
}

RegisterIndex BytecodeOptimizer::DecodeA(Instruction inst) const {
    return static_cast<RegisterIndex>((inst >> 6) & 0xFF);
}

int BytecodeOptimizer::DecodeB(Instruction inst) const {
    return static_cast<int>((inst >> 14) & 0x1FF);
}

int BytecodeOptimizer::DecodeC(Instruction inst) const {
    return static_cast<int>((inst >> 23) & 0x1FF);
}

int BytecodeOptimizer::DecodeBx(Instruction inst) const {
    return static_cast<int>((inst >> 14) & 0x3FFFF);
}

int BytecodeOptimizer::DecodeSBx(Instruction inst) const {
    constexpr int MAXARG_sBx = 0x20000 - 1; // (1 << 18) / 2 - 1
    return DecodeBx(inst) - MAXARG_sBx;
}

// 指令编码辅助函数
Instruction BytecodeOptimizer::EncodeABC(OpCode op, RegisterIndex a, int b, int c) const {
    return static_cast<Instruction>(op) |
           (static_cast<Instruction>(a) << 6) |
           (static_cast<Instruction>(b) << 14) |
           (static_cast<Instruction>(c) << 23);
}

Instruction BytecodeOptimizer::EncodeABx(OpCode op, RegisterIndex a, int bx) const {
    return static_cast<Instruction>(op) |
           (static_cast<Instruction>(a) << 6) |
           (static_cast<Instruction>(bx) << 14);
}

Instruction BytecodeOptimizer::EncodeAsBx(OpCode op, RegisterIndex a, int sbx) const {
    constexpr int MAXARG_sBx = 0x20000 - 1; // (1 << 18) / 2 - 1
    return static_cast<Instruction>(op) |
           (static_cast<Instruction>(a) << 6) |
           (static_cast<Instruction>(sbx + MAXARG_sBx) << 14);
}

} // namespace lua_cpp