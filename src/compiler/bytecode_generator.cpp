/**
 * @file bytecode_generator.cpp
 * @brief 字节码生成器实现
 * @description 负责生成Lua 5.1.5兼容的字节码指令
 * @author Lua C++ Project
 * @date 2025-09-25
 */

#include "bytecode_generator.h"
#include "../core/lua_errors.h"
#include <algorithm>
#include <sstream>

namespace lua_cpp {

/* ========================================================================== */
/* BytecodeGenerator 实现 */
/* ========================================================================== */

BytecodeGenerator::BytecodeGenerator() : current_line_(1) {
}

BytecodeGenerator::~BytecodeGenerator() = default;

Size BytecodeGenerator::EmitInstruction(Instruction instruction, int line) {
    instructions_.push_back(instruction);
    line_info_.push_back(line > 0 ? line : current_line_);
    return instructions_.size() - 1;
}

Size BytecodeGenerator::EmitABC(OpCode op, int a, int b, int c, int line) {
    Instruction inst = CreateABC(op, a, b, c);
    return EmitInstruction(inst, line);
}

Size BytecodeGenerator::EmitABx(OpCode op, int a, int bx, int line) {
    Instruction inst = CreateABx(op, a, bx);
    return EmitInstruction(inst, line);
}

Size BytecodeGenerator::EmitAsBx(OpCode op, int a, int sbx, int line) {
    Instruction inst = CreateAsBx(op, a, sbx);
    return EmitInstruction(inst, line);
}

Size BytecodeGenerator::EmitJump(OpCode op, int a) {
    // 发射跳转指令，暂时使用0作为占位符
    return EmitAsBx(op, a, 0);
}

void BytecodeGenerator::PatchJump(Size pc, Size target) {
    if (pc >= instructions_.size()) {
        throw CompilerError("Invalid jump instruction PC: " + std::to_string(pc));
    }
    
    Instruction& inst = instructions_[pc];
    OpCode op = GetOpCode(inst);
    int a = GetArgA(inst);
    
    // 计算相对跳转偏移
    int offset = static_cast<int>(target) - static_cast<int>(pc) - 1;
    
    // 检查跳转范围
    if (offset < -MAXARG_sBx || offset > MAXARG_sBx) {
        throw CompilerError("Jump offset out of range: " + std::to_string(offset));
    }
    
    inst = CreateAsBx(op, a, offset);
}

void BytecodeGenerator::PatchJumpToHere(Size pc) {
    PatchJump(pc, GetCurrentPC());
}

Size BytecodeGenerator::GetCurrentPC() const {
    return instructions_.size();
}

void BytecodeGenerator::SetCurrentLine(int line) {
    current_line_ = line;
}

int BytecodeGenerator::GetCurrentLine() const {
    return current_line_;
}

const std::vector<Instruction>& BytecodeGenerator::GetInstructions() const {
    return instructions_;
}

const std::vector<int>& BytecodeGenerator::GetLineInfo() const {
    return line_info_;
}

void BytecodeGenerator::ReserveInstructions(Size count) {
    instructions_.reserve(instructions_.size() + count);
    line_info_.reserve(line_info_.size() + count);
}

Instruction BytecodeGenerator::GetInstruction(Size pc) const {
    if (pc >= instructions_.size()) {
        throw CompilerError("Invalid instruction PC: " + std::to_string(pc));
    }
    return instructions_[pc];
}

void BytecodeGenerator::SetInstruction(Size pc, Instruction instruction) {
    if (pc >= instructions_.size()) {
        throw CompilerError("Invalid instruction PC: " + std::to_string(pc));
    }
    instructions_[pc] = instruction;
}

bool BytecodeGenerator::IsValidJumpTarget(Size pc) const {
    return pc <= instructions_.size();
}

std::string BytecodeGenerator::InstructionToString(Instruction inst) const {
    OpCode op = GetOpCode(inst);
    int a = GetArgA(inst);
    
    std::ostringstream oss;
    oss << OPCODE_INFO[static_cast<int>(op)].name;
    
    switch (OPCODE_INFO[static_cast<int>(op)].mode) {
        case InstructionMode::iABC: {
            int b = GetArgB(inst);
            int c = GetArgC(inst);
            oss << " " << a << " " << b << " " << c;
            break;
        }
        case InstructionMode::iABx: {
            int bx = GetArgBx(inst);
            oss << " " << a << " " << bx;
            break;
        }
        case InstructionMode::iAsBx: {
            int sbx = GetArgsBx(inst);
            oss << " " << a << " " << sbx;
            break;
        }
    }
    
    return oss.str();
}

void BytecodeGenerator::Clear() {
    instructions_.clear();
    line_info_.clear();
    current_line_ = 1;
}

/* ========================================================================== */
/* InstructionEmitter 实现 */
/* ========================================================================== */

InstructionEmitter::InstructionEmitter(BytecodeGenerator& generator)
    : generator_(generator) {
}

Size InstructionEmitter::EmitMove(RegisterIndex dst, RegisterIndex src) {
    return generator_.EmitABC(OpCode::MOVE, dst, src, 0);
}

Size InstructionEmitter::EmitLoadK(RegisterIndex dst, int constant_index) {
    if (constant_index < 0 || constant_index > MAXARG_Bx) {
        throw CompilerError("Constant index out of range: " + std::to_string(constant_index));
    }
    return generator_.EmitABx(OpCode::LOADK, dst, constant_index);
}

Size InstructionEmitter::EmitLoadBool(RegisterIndex dst, bool value, bool skip) {
    return generator_.EmitABC(OpCode::LOADBOOL, dst, value ? 1 : 0, skip ? 1 : 0);
}

Size InstructionEmitter::EmitLoadNil(RegisterIndex start, RegisterIndex end) {
    return generator_.EmitABC(OpCode::LOADNIL, start, end, 0);
}

Size InstructionEmitter::EmitGetGlobal(RegisterIndex dst, int name_index) {
    return generator_.EmitABx(OpCode::GETGLOBAL, dst, name_index);
}

Size InstructionEmitter::EmitSetGlobal(RegisterIndex src, int name_index) {
    return generator_.EmitABx(OpCode::SETGLOBAL, src, name_index);
}

Size InstructionEmitter::EmitGetTable(RegisterIndex dst, RegisterIndex table, int key_rk) {
    return generator_.EmitABC(OpCode::GETTABLE, dst, table, key_rk);
}

Size InstructionEmitter::EmitSetTable(RegisterIndex table, int key_rk, int value_rk) {
    return generator_.EmitABC(OpCode::SETTABLE, table, key_rk, value_rk);
}

Size InstructionEmitter::EmitNewTable(RegisterIndex dst, int array_size, int hash_size) {
    return generator_.EmitABC(OpCode::NEWTABLE, dst, array_size, hash_size);
}

Size InstructionEmitter::EmitAdd(RegisterIndex dst, int left_rk, int right_rk) {
    return generator_.EmitABC(OpCode::ADD, dst, left_rk, right_rk);
}

Size InstructionEmitter::EmitSub(RegisterIndex dst, int left_rk, int right_rk) {
    return generator_.EmitABC(OpCode::SUB, dst, left_rk, right_rk);
}

Size InstructionEmitter::EmitMul(RegisterIndex dst, int left_rk, int right_rk) {
    return generator_.EmitABC(OpCode::MUL, dst, left_rk, right_rk);
}

Size InstructionEmitter::EmitDiv(RegisterIndex dst, int left_rk, int right_rk) {
    return generator_.EmitABC(OpCode::DIV, dst, left_rk, right_rk);
}

Size InstructionEmitter::EmitMod(RegisterIndex dst, int left_rk, int right_rk) {
    return generator_.EmitABC(OpCode::MOD, dst, left_rk, right_rk);
}

Size InstructionEmitter::EmitPow(RegisterIndex dst, int left_rk, int right_rk) {
    return generator_.EmitABC(OpCode::POW, dst, left_rk, right_rk);
}

Size InstructionEmitter::EmitUnm(RegisterIndex dst, RegisterIndex src) {
    return generator_.EmitABC(OpCode::UNM, dst, src, 0);
}

Size InstructionEmitter::EmitNot(RegisterIndex dst, RegisterIndex src) {
    return generator_.EmitABC(OpCode::NOT, dst, src, 0);
}

Size InstructionEmitter::EmitLen(RegisterIndex dst, RegisterIndex src) {
    return generator_.EmitABC(OpCode::LEN, dst, src, 0);
}

Size InstructionEmitter::EmitConcat(RegisterIndex dst, RegisterIndex start, RegisterIndex end) {
    return generator_.EmitABC(OpCode::CONCAT, dst, start, end);
}

Size InstructionEmitter::EmitJump(int offset) {
    return generator_.EmitAsBx(OpCode::JMP, 0, offset);
}

Size InstructionEmitter::EmitJumpPlaceholder() {
    return generator_.EmitJump(OpCode::JMP, 0);
}

Size InstructionEmitter::EmitEq(bool invert, int left_rk, int right_rk) {
    return generator_.EmitABC(OpCode::EQ, invert ? 1 : 0, left_rk, right_rk);
}

Size InstructionEmitter::EmitLt(bool invert, int left_rk, int right_rk) {
    return generator_.EmitABC(OpCode::LT, invert ? 1 : 0, left_rk, right_rk);
}

Size InstructionEmitter::EmitLe(bool invert, int left_rk, int right_rk) {
    return generator_.EmitABC(OpCode::LE, invert ? 1 : 0, left_rk, right_rk);
}

Size InstructionEmitter::EmitTest(RegisterIndex condition, bool invert) {
    return generator_.EmitABC(OpCode::TEST, condition, 0, invert ? 1 : 0);
}

Size InstructionEmitter::EmitTestSet(RegisterIndex dst, RegisterIndex condition, bool invert) {
    return generator_.EmitABC(OpCode::TESTSET, dst, condition, invert ? 1 : 0);
}

Size InstructionEmitter::EmitCall(RegisterIndex func, int num_args, int num_results) {
    return generator_.EmitABC(OpCode::CALL, func, num_args, num_results);
}

Size InstructionEmitter::EmitTailCall(RegisterIndex func, int num_args) {
    return generator_.EmitABC(OpCode::TAILCALL, func, num_args, 0);
}

Size InstructionEmitter::EmitReturn(RegisterIndex start, int count) {
    return generator_.EmitABC(OpCode::RETURN, start, count, 0);
}

Size InstructionEmitter::EmitForPrep(RegisterIndex base, int jump_offset) {
    return generator_.EmitAsBx(OpCode::FORPREP, base, jump_offset);
}

Size InstructionEmitter::EmitForLoop(RegisterIndex base, int jump_offset) {
    return generator_.EmitAsBx(OpCode::FORLOOP, base, jump_offset);
}

Size InstructionEmitter::EmitTForLoop(RegisterIndex base, int jump_offset) {
    return generator_.EmitAsBx(OpCode::TFORLOOP, base, jump_offset);
}

Size InstructionEmitter::EmitSetList(RegisterIndex table, int batch, int count) {
    return generator_.EmitABC(OpCode::SETLIST, table, batch, count);
}

Size InstructionEmitter::EmitClose(RegisterIndex start) {
    return generator_.EmitABC(OpCode::CLOSE, start, 0, 0);
}

Size InstructionEmitter::EmitClosure(RegisterIndex dst, int proto_index) {
    return generator_.EmitABx(OpCode::CLOSURE, dst, proto_index);
}

Size InstructionEmitter::EmitVararg(RegisterIndex dst, int count) {
    return generator_.EmitABC(OpCode::VARARG, dst, count, 0);
}

Size InstructionEmitter::EmitGetUpval(RegisterIndex dst, int upval_index) {
    return generator_.EmitABC(OpCode::GETUPVAL, dst, upval_index, 0);
}

Size InstructionEmitter::EmitSetUpval(RegisterIndex src, int upval_index) {
    return generator_.EmitABC(OpCode::SETUPVAL, src, upval_index, 0);
}

/* ========================================================================== */
/* 辅助函数实现 */
/* ========================================================================== */

bool IsValidRegister(RegisterIndex reg) {
    return reg >= 0 && reg <= MAXARG_A;
}

bool IsValidConstantIndex(int index) {
    return index >= 0 && index <= MAXARG_C;
}

bool IsValidRK(int rk) {
    if (IsConstant(rk)) {
        return IsValidConstantIndex(RKToConstantIndex(rk));
    } else {
        return IsValidRegister(RKToRegisterIndex(rk));
    }
}

int EncodeRK(RegisterIndex reg) {
    if (!IsValidRegister(reg)) {
        throw CompilerError("Invalid register for RK encoding: " + std::to_string(reg));
    }
    return RegisterIndexToRK(reg);
}

int EncodeRK(int constant_index) {
    if (!IsValidConstantIndex(constant_index)) {
        throw CompilerError("Invalid constant index for RK encoding: " + std::to_string(constant_index));
    }
    return ConstantIndexToRK(constant_index);
}

std::string DecodeInstruction(Instruction inst) {
    OpCode op = GetOpCode(inst);
    const OpCodeInfo& info = OPCODE_INFO[static_cast<int>(op)];
    
    std::ostringstream oss;
    oss << info.name;
    
    int a = GetArgA(inst);
    oss << " " << a;
    
    switch (info.mode) {
        case InstructionMode::iABC: {
            int b = GetArgB(inst);
            int c = GetArgC(inst);
            oss << " " << b << " " << c;
            break;
        }
        case InstructionMode::iABx: {
            int bx = GetArgBx(inst);
            oss << " " << bx;
            break;
        }
        case InstructionMode::iAsBx: {
            int sbx = GetArgsBx(inst);
            oss << " " << sbx;
            break;
        }
    }
    
    return oss.str();
}

} // namespace lua_cpp