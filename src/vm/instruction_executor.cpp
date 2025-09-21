/**
 * @file instruction_executor.cpp
 * @brief Lua虚拟机指令执行器实现
 * @description 实现所有Lua 5.1.5字节码指令的执行逻辑
 * @author Lua C++ Project
 * @date 2025-09-21
 */

#include "virtual_machine.h"
#include "../types/value.h"
#include <cmath>
#include <string>

namespace lua_cpp {

/* ========================================================================== */
/* 数据移动指令 */
/* ========================================================================== */

void VirtualMachine::ExecuteMOVE(RegisterIndex a, int b) {
    // MOVE A B: R(A) := R(B)
    LuaValue value = GetRegister(static_cast<RegisterIndex>(b));
    SetRegister(a, value);
}

void VirtualMachine::ExecuteLOADK(RegisterIndex a, int bx) {
    // LOADK A Bx: R(A) := Kst(Bx)
    if (!current_proto_ || bx >= current_proto_->constants.size()) {
        throw VMExecutionError("Invalid constant index in LOADK");
    }
    
    SetRegister(a, current_proto_->constants[bx]);
}

void VirtualMachine::ExecuteLOADBOOL(RegisterIndex a, int b, int c) {
    // LOADBOOL A B C: R(A) := (Bool)B; if (C) pc++
    SetRegister(a, LuaValue(b != 0));
    
    if (c != 0) {
        instruction_pointer_++; // 跳过下一条指令
    }
}

void VirtualMachine::ExecuteLOADNIL(RegisterIndex a, int b) {
    // LOADNIL A B: R(A), R(A+1), ..., R(A+B) := nil
    for (int i = 0; i <= b; i++) {
        SetRegister(a + i, LuaValue());
    }
}

/* ========================================================================== */
/* 全局变量和上值操作 */
/* ========================================================================== */

void VirtualMachine::ExecuteGETUPVAL(RegisterIndex a, int b) {
    // GETUPVAL A B: R(A) := UpValue[B]
    // TODO: 实现上值访问
    // 暂时设为nil
    SetRegister(a, LuaValue());
}

void VirtualMachine::ExecuteGETGLOBAL(RegisterIndex a, int bx) {
    // GETGLOBAL A Bx: R(A) := Gbl[Kst(Bx)]
    if (!current_proto_ || bx >= current_proto_->constants.size()) {
        throw VMExecutionError("Invalid constant index in GETGLOBAL");
    }
    
    const LuaValue& key = current_proto_->constants[bx];
    if (!key.IsString()) {
        throw TypeError("Global variable name must be a string");
    }
    
    // TODO: 从全局表中获取值
    // 暂时返回nil
    SetRegister(a, LuaValue());
}

void VirtualMachine::ExecuteSETGLOBAL(RegisterIndex a, int bx) {
    // SETGLOBAL A Bx: Gbl[Kst(Bx)] := R(A)
    if (!current_proto_ || bx >= current_proto_->constants.size()) {
        throw VMExecutionError("Invalid constant index in SETGLOBAL");
    }
    
    const LuaValue& key = current_proto_->constants[bx];
    if (!key.IsString()) {
        throw TypeError("Global variable name must be a string");
    }
    
    LuaValue value = GetRegister(a);
    
    // TODO: 设置全局表中的值
}

void VirtualMachine::ExecuteSETUPVAL(RegisterIndex a, int b) {
    // SETUPVAL A B: UpValue[B] := R(A)
    LuaValue value = GetRegister(a);
    
    // TODO: 实现上值设置
}

/* ========================================================================== */
/* 表操作指令 */
/* ========================================================================== */

void VirtualMachine::ExecuteGETTABLE(RegisterIndex a, int b, int c) {
    // GETTABLE A B C: R(A) := R(B)[RK(C)]
    LuaValue table = GetRegister(static_cast<RegisterIndex>(b));
    LuaValue key = GetRK(c);
    
    if (!table.IsTable()) {
        throw TypeError("Attempt to index a " + table.TypeName() + " value");
    }
    
    // TODO: 实现表索引操作
    // 暂时返回nil
    SetRegister(a, LuaValue());
    
    statistics_.table_operations++;
}

void VirtualMachine::ExecuteSETTABLE(RegisterIndex a, int b, int c) {
    // SETTABLE A B C: R(A)[RK(B)] := RK(C)
    LuaValue table = GetRegister(a);
    LuaValue key = GetRK(b);
    LuaValue value = GetRK(c);
    
    if (!table.IsTable()) {
        throw TypeError("Attempt to index a " + table.TypeName() + " value");
    }
    
    // TODO: 实现表赋值操作
    
    statistics_.table_operations++;
}

void VirtualMachine::ExecuteNEWTABLE(RegisterIndex a, int b, int c) {
    // NEWTABLE A B C: R(A) := {} (size = B,C)
    // TODO: 创建新表
    // 暂时创建空的LuaValue
    SetRegister(a, LuaValue());
    
    statistics_.table_operations++;
}

void VirtualMachine::ExecuteSELF(RegisterIndex a, int b, int c) {
    // SELF A B C: R(A+1) := R(B); R(A) := R(B)[RK(C)]
    LuaValue table = GetRegister(static_cast<RegisterIndex>(b));
    LuaValue key = GetRK(c);
    
    SetRegister(a + 1, table);
    
    if (!table.IsTable()) {
        throw TypeError("Attempt to index a " + table.TypeName() + " value");
    }
    
    // TODO: 获取方法
    SetRegister(a, LuaValue());
    
    statistics_.table_operations++;
}

/* ========================================================================== */
/* 算术运算指令 */
/* ========================================================================== */

void VirtualMachine::ExecuteADD(RegisterIndex a, int b, int c) {
    // ADD A B C: R(A) := RK(B) + RK(C)
    LuaValue left = GetRK(b);
    LuaValue right = GetRK(c);
    
    if (left.IsNumber() && right.IsNumber()) {
        double result = left.AsNumber() + right.AsNumber();
        SetRegister(a, LuaValue(result));
    } else {
        // TODO: 元方法处理
        throw TypeError("Attempt to perform arithmetic on non-number values");
    }
}

void VirtualMachine::ExecuteSUB(RegisterIndex a, int b, int c) {
    // SUB A B C: R(A) := RK(B) - RK(C)
    LuaValue left = GetRK(b);
    LuaValue right = GetRK(c);
    
    if (left.IsNumber() && right.IsNumber()) {
        double result = left.AsNumber() - right.AsNumber();
        SetRegister(a, LuaValue(result));
    } else {
        throw TypeError("Attempt to perform arithmetic on non-number values");
    }
}

void VirtualMachine::ExecuteMUL(RegisterIndex a, int b, int c) {
    // MUL A B C: R(A) := RK(B) * RK(C)
    LuaValue left = GetRK(b);
    LuaValue right = GetRK(c);
    
    if (left.IsNumber() && right.IsNumber()) {
        double result = left.AsNumber() * right.AsNumber();
        SetRegister(a, LuaValue(result));
    } else {
        throw TypeError("Attempt to perform arithmetic on non-number values");
    }
}

void VirtualMachine::ExecuteDIV(RegisterIndex a, int b, int c) {
    // DIV A B C: R(A) := RK(B) / RK(C)
    LuaValue left = GetRK(b);
    LuaValue right = GetRK(c);
    
    if (left.IsNumber() && right.IsNumber()) {
        double divisor = right.AsNumber();
        if (divisor == 0.0) {
            throw VMExecutionError("Division by zero");
        }
        double result = left.AsNumber() / divisor;
        SetRegister(a, LuaValue(result));
    } else {
        throw TypeError("Attempt to perform arithmetic on non-number values");
    }
}

void VirtualMachine::ExecuteMOD(RegisterIndex a, int b, int c) {
    // MOD A B C: R(A) := RK(B) % RK(C)
    LuaValue left = GetRK(b);
    LuaValue right = GetRK(c);
    
    if (left.IsNumber() && right.IsNumber()) {
        double divisor = right.AsNumber();
        if (divisor == 0.0) {
            throw VMExecutionError("Division by zero in modulo operation");
        }
        double result = fmod(left.AsNumber(), divisor);
        SetRegister(a, LuaValue(result));
    } else {
        throw TypeError("Attempt to perform arithmetic on non-number values");
    }
}

void VirtualMachine::ExecutePOW(RegisterIndex a, int b, int c) {
    // POW A B C: R(A) := RK(B) ^ RK(C)
    LuaValue left = GetRK(b);
    LuaValue right = GetRK(c);
    
    if (left.IsNumber() && right.IsNumber()) {
        double result = pow(left.AsNumber(), right.AsNumber());
        SetRegister(a, LuaValue(result));
    } else {
        throw TypeError("Attempt to perform arithmetic on non-number values");
    }
}

void VirtualMachine::ExecuteUNM(RegisterIndex a, int b) {
    // UNM A B: R(A) := -R(B)
    LuaValue value = GetRegister(static_cast<RegisterIndex>(b));
    
    if (value.IsNumber()) {
        SetRegister(a, LuaValue(-value.AsNumber()));
    } else {
        throw TypeError("Attempt to perform arithmetic on non-number value");
    }
}

/* ========================================================================== */
/* 逻辑运算指令 */
/* ========================================================================== */

void VirtualMachine::ExecuteNOT(RegisterIndex a, int b) {
    // NOT A B: R(A) := not R(B)
    LuaValue value = GetRegister(static_cast<RegisterIndex>(b));
    SetRegister(a, LuaValue(!value.IsTruthy()));
}

void VirtualMachine::ExecuteLEN(RegisterIndex a, int b) {
    // LEN A B: R(A) := length of R(B)
    LuaValue value = GetRegister(static_cast<RegisterIndex>(b));
    
    if (value.IsString()) {
        SetRegister(a, LuaValue(static_cast<double>(value.AsString().length())));
    } else if (value.IsTable()) {
        // TODO: 实现表长度计算
        SetRegister(a, LuaValue(0.0));
    } else {
        throw TypeError("Attempt to get length of a " + value.TypeName() + " value");
    }
}

void VirtualMachine::ExecuteCONCAT(RegisterIndex a, int b, int c) {
    // CONCAT A B C: R(A) := R(B).. ... ..R(C)
    std::string result;
    
    for (int i = b; i <= c; i++) {
        LuaValue value = GetRegister(static_cast<RegisterIndex>(i));
        
        if (value.IsString()) {
            result += value.AsString();
        } else if (value.IsNumber()) {
            result += std::to_string(value.AsNumber());
        } else {
            throw TypeError("Attempt to concatenate a " + value.TypeName() + " value");
        }
    }
    
    SetRegister(a, LuaValue(result));
}

/* ========================================================================== */
/* 跳转和条件指令 */
/* ========================================================================== */

void VirtualMachine::ExecuteJMP(int sbx) {
    // JMP sBx: pc += sBx
    instruction_pointer_ += sbx;
}

void VirtualMachine::ExecuteEQ(RegisterIndex a, int b, int c) {
    // EQ A B C: if ((RK(B) == RK(C)) ~= A) then pc++
    LuaValue left = GetRK(b);
    LuaValue right = GetRK(c);
    
    bool equal = (left == right);
    
    if ((equal ? 1 : 0) != a) {
        instruction_pointer_++; // 跳过下一条指令
    }
}

void VirtualMachine::ExecuteLT(RegisterIndex a, int b, int c) {
    // LT A B C: if ((RK(B) < RK(C)) ~= A) then pc++
    LuaValue left = GetRK(b);
    LuaValue right = GetRK(c);
    
    bool less_than = false;
    
    if (left.IsNumber() && right.IsNumber()) {
        less_than = left.AsNumber() < right.AsNumber();
    } else if (left.IsString() && right.IsString()) {
        less_than = left.AsString() < right.AsString();
    } else {
        throw TypeError("Attempt to compare " + left.TypeName() + " with " + right.TypeName());
    }
    
    if ((less_than ? 1 : 0) != a) {
        instruction_pointer_++;
    }
}

void VirtualMachine::ExecuteLE(RegisterIndex a, int b, int c) {
    // LE A B C: if ((RK(B) <= RK(C)) ~= A) then pc++
    LuaValue left = GetRK(b);
    LuaValue right = GetRK(c);
    
    bool less_equal = false;
    
    if (left.IsNumber() && right.IsNumber()) {
        less_equal = left.AsNumber() <= right.AsNumber();
    } else if (left.IsString() && right.IsString()) {
        less_equal = left.AsString() <= right.AsString();
    } else {
        throw TypeError("Attempt to compare " + left.TypeName() + " with " + right.TypeName());
    }
    
    if ((less_equal ? 1 : 0) != a) {
        instruction_pointer_++;
    }
}

void VirtualMachine::ExecuteTEST(RegisterIndex a, int c) {
    // TEST A C: if not (R(A) <=> C) then pc++
    LuaValue value = GetRegister(a);
    bool test_result = value.IsTruthy();
    
    if (test_result != (c != 0)) {
        instruction_pointer_++;
    }
}

void VirtualMachine::ExecuteTESTSET(RegisterIndex a, int b, int c) {
    // TESTSET A B C: if (R(B) <=> C) then R(A) := R(B) else pc++
    LuaValue value = GetRegister(static_cast<RegisterIndex>(b));
    bool test_result = value.IsTruthy();
    
    if (test_result == (c != 0)) {
        SetRegister(a, value);
    } else {
        instruction_pointer_++;
    }
}

/* ========================================================================== */
/* 函数调用指令 */
/* ========================================================================== */

void VirtualMachine::ExecuteCALL(RegisterIndex a, int b, int c) {
    // CALL A B C: R(A), ... ,R(A+C-2) := R(A)(R(A+1), ... ,R(A+B-1))
    LuaValue function = GetRegister(a);
    
    if (!function.IsFunction()) {
        throw TypeError("Attempt to call a " + function.TypeName() + " value");
    }
    
    // TODO: 实现函数调用
    // 1. 准备参数
    // 2. 创建新的调用帧
    // 3. 执行函数
    // 4. 处理返回值
    
    // 暂时什么也不做
    statistics_.function_calls++;
}

void VirtualMachine::ExecuteTAILCALL(RegisterIndex a, int b, int c) {
    // TAILCALL A B C: return R(A)(R(A+1), ... ,R(A+B-1))
    LuaValue function = GetRegister(a);
    
    if (!function.IsFunction()) {
        throw TypeError("Attempt to call a " + function.TypeName() + " value");
    }
    
    // TODO: 实现尾调用
    // 尾调用不会增加调用栈深度
    
    statistics_.function_calls++;
}

void VirtualMachine::ExecuteRETURN(RegisterIndex a, int b) {
    // RETURN A B: return R(A), ... ,R(A+B-2)
    
    // 收集返回值
    std::vector<LuaValue> return_values;
    
    if (b == 0) {
        // 返回从A到栈顶的所有值
        Size stack_top = stack_->Size();
        Size base = GetCurrentBase();
        for (Size i = base + a; i < stack_top; i++) {
            return_values.push_back(stack_->Get(i));
        }
    } else {
        // 返回指定数量的值
        for (int i = 0; i < b - 1; i++) {
            return_values.push_back(GetRegister(a + i));
        }
    }
    
    // 弹出当前调用帧
    PopCallFrame();
    
    // 如果不是主函数，将返回值放到调用者的栈中
    if (!call_stack_.empty()) {
        // TODO: 将返回值放到正确的位置
    } else {
        // 主函数返回，将返回值放到栈顶
        for (const auto& value : return_values) {
            stack_->Push(value);
        }
    }
}

/* ========================================================================== */
/* 循环指令 */
/* ========================================================================== */

void VirtualMachine::ExecuteFORLOOP(RegisterIndex a, int sbx) {
    // FORLOOP A sBx: R(A) += R(A+2); if R(A) <?= R(A+1) then { pc += sBx; R(A+3) = R(A) }
    LuaValue init = GetRegister(a);
    LuaValue limit = GetRegister(a + 1);
    LuaValue step = GetRegister(a + 2);
    
    if (!init.IsNumber() || !limit.IsNumber() || !step.IsNumber()) {
        throw TypeError("For loop variables must be numbers");
    }
    
    double new_init = init.AsNumber() + step.AsNumber();
    SetRegister(a, LuaValue(new_init));
    
    bool continue_loop;
    if (step.AsNumber() > 0) {
        continue_loop = new_init <= limit.AsNumber();
    } else {
        continue_loop = new_init >= limit.AsNumber();
    }
    
    if (continue_loop) {
        instruction_pointer_ += sbx;
        SetRegister(a + 3, LuaValue(new_init)); // 循环变量
    }
}

void VirtualMachine::ExecuteFORPREP(RegisterIndex a, int sbx) {
    // FORPREP A sBx: R(A) -= R(A+2); pc += sBx
    LuaValue init = GetRegister(a);
    LuaValue step = GetRegister(a + 2);
    
    if (!init.IsNumber() || !step.IsNumber()) {
        throw TypeError("For loop variables must be numbers");
    }
    
    double new_init = init.AsNumber() - step.AsNumber();
    SetRegister(a, LuaValue(new_init));
    
    instruction_pointer_ += sbx;
}

void VirtualMachine::ExecuteTFORLOOP(RegisterIndex a, int c) {
    // TFORLOOP A C: R(A+3), ... ,R(A+2+C) := R(A)(R(A+1), R(A+2)); if R(A+3) ~= nil then R(A+2) = R(A+3) else pc++
    // TODO: 实现通用for循环
    instruction_pointer_++; // 暂时跳过
}

/* ========================================================================== */
/* 其他指令 */
/* ========================================================================== */

void VirtualMachine::ExecuteSETLIST(RegisterIndex a, int b, int c) {
    // SETLIST A B C: R(A)[(C-1)*FPF+i] := R(A+i), 1 <= i <= B
    LuaValue table = GetRegister(a);
    
    if (!table.IsTable()) {
        throw TypeError("Attempt to use SETLIST on non-table value");
    }
    
    // TODO: 实现列表设置
    
    statistics_.table_operations++;
}

void VirtualMachine::ExecuteCLOSE(RegisterIndex a) {
    // CLOSE A: close all variables in the stack up to (>=) R(A)
    // TODO: 实现上值关闭
}

void VirtualMachine::ExecuteCLOSURE(RegisterIndex a, int bx) {
    // CLOSURE A Bx: R(A) := closure(KPROTO[Bx])
    // TODO: 实现闭包创建
    
    if (!current_proto_ || bx >= current_proto_->protos.size()) {
        throw VMExecutionError("Invalid proto index in CLOSURE");
    }
    
    // 暂时设为nil
    SetRegister(a, LuaValue());
}

void VirtualMachine::ExecuteVARARG(RegisterIndex a, int b) {
    // VARARG A B: R(A), R(A+1), ..., R(A+B-2) := vararg
    // TODO: 实现变参处理
    
    if (b == 0) {
        // 将所有变参复制到寄存器
    } else {
        // 复制指定数量的变参
        for (int i = 0; i < b - 1; i++) {
            SetRegister(a + i, LuaValue()); // 暂时设为nil
        }
    }
}

} // namespace lua_cpp