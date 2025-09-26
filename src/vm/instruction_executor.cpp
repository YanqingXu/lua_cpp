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
    SetRegister(a, std::move(value));
}

void VirtualMachine::ExecuteLOADK(RegisterIndex a, int bx) {
    // LOADK A Bx: R(A) := Kst(Bx)
    if (!current_proto_ || static_cast<Size>(bx) >= current_proto_->GetConstantCount()) {
        throw VMExecutionError("Invalid constant index in LOADK: " + std::to_string(bx));
    }
    
    SetRegister(a, current_proto_->GetConstant(bx));
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
    if (!current_proto_ || static_cast<Size>(bx) >= current_proto_->GetConstantCount()) {
        throw VMExecutionError("Invalid constant index in GETGLOBAL: " + std::to_string(bx));
    }
    
    const LuaValue& key = current_proto_->GetConstant(bx);
    if (!key.IsString()) {
        throw TypeError("Global variable name must be a string");
    }
    
    // 从全局表中获取值
    if (global_table_) {
        LuaValue value = global_table_->Get(key);
        SetRegister(a, std::move(value));
    } else {
        SetRegister(a, LuaValue()); // nil
    }
}

void VirtualMachine::ExecuteSETGLOBAL(RegisterIndex a, int bx) {
    // SETGLOBAL A Bx: Gbl[Kst(Bx)] := R(A)
    if (!current_proto_ || static_cast<Size>(bx) >= current_proto_->GetConstantCount()) {
        throw VMExecutionError("Invalid constant index in SETGLOBAL: " + std::to_string(bx));
    }
    
    const LuaValue& key = current_proto_->GetConstant(bx);
    if (!key.IsString()) {
        throw TypeError("Global variable name must be a string");
    }
    
    LuaValue value = GetRegister(a);
    
    // 设置全局表中的值
    if (global_table_) {
        global_table_->Set(key, std::move(value));
    }
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
    
    // 从表中获取值
    auto table_ptr = table.GetTable();
    if (table_ptr) {
        LuaValue result = table_ptr->Get(key);
        SetRegister(a, std::move(result));
    } else {
        SetRegister(a, LuaValue()); // nil
    }
    
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
    
    // 设置表中的值
    auto table_ptr = table.GetTable();
    if (table_ptr) {
        table_ptr->Set(key, std::move(value));
    }
    
    statistics_.table_operations++;
}

void VirtualMachine::ExecuteNEWTABLE(RegisterIndex a, int b, int c) {
    // NEWTABLE A B C: R(A) := {} (size = B,C)
    // b = 数组部分大小, c = 哈希部分大小
    Size array_size = (b == 0) ? 0 : (1 << (b - 1));
    Size hash_size = (c == 0) ? 0 : (1 << (c - 1));
    
    auto new_table = std::make_shared<LuaTable>(array_size, hash_size);
    SetRegister(a, LuaValue(new_table));
    
    statistics_.table_operations++;
}

void VirtualMachine::ExecuteSELF(RegisterIndex a, int b, int c) {
    // SELF A B C: R(A+1) := R(B); R(A) := R(B)[RK(C)]
    LuaValue table = GetRegister(static_cast<RegisterIndex>(b));
    LuaValue key = GetRK(c);
    
    // 将表对象复制到 R(A+1) 作为 self 参数
    SetRegister(a + 1, table);
    
    if (!table.IsTable()) {
        throw TypeError("Attempt to index a " + table.TypeName() + " value");
    }
    
    // 获取方法并存储到 R(A)
    auto table_ptr = table.GetTable();
    if (table_ptr) {
        LuaValue method = table_ptr->Get(key);
        SetRegister(a, std::move(method));
    } else {
        SetRegister(a, LuaValue()); // nil
    }
    
    statistics_.table_operations++;
}

/* ========================================================================== */
/* 算术运算指令 */
/* ========================================================================== */

void VirtualMachine::ExecuteADD(RegisterIndex a, int b, int c) {
    // ADD A B C: R(A) := RK(B) + RK(C)
    LuaValue left = GetRK(b);
    LuaValue right = GetRK(c);
    
    // 尝试转换为数字
    std::optional<double> left_num = left.ToNumber();
    std::optional<double> right_num = right.ToNumber();
    
    if (left_num && right_num) {
        double result = *left_num + *right_num;
        SetRegister(a, LuaValue(result));
    } else {
        // TODO: 元方法处理
        throw TypeError("Attempt to perform arithmetic (" + left.TypeName() + 
                       " + " + right.TypeName() + ")");
    }
}

void VirtualMachine::ExecuteSUB(RegisterIndex a, int b, int c) {
    // SUB A B C: R(A) := RK(B) - RK(C)
    LuaValue left = GetRK(b);
    LuaValue right = GetRK(c);
    
    std::optional<double> left_num = left.ToNumber();
    std::optional<double> right_num = right.ToNumber();
    
    if (left_num && right_num) {
        double result = *left_num - *right_num;
        SetRegister(a, LuaValue(result));
    } else {
        throw TypeError("Attempt to perform arithmetic (" + left.TypeName() + 
                       " - " + right.TypeName() + ")");
    }
}

void VirtualMachine::ExecuteMUL(RegisterIndex a, int b, int c) {
    // MUL A B C: R(A) := RK(B) * RK(C)
    LuaValue left = GetRK(b);
    LuaValue right = GetRK(c);
    
    std::optional<double> left_num = left.ToNumber();
    std::optional<double> right_num = right.ToNumber();
    
    if (left_num && right_num) {
        double result = *left_num * *right_num;
        SetRegister(a, LuaValue(result));
    } else {
        throw TypeError("Attempt to perform arithmetic (" + left.TypeName() + 
                       " * " + right.TypeName() + ")");
    }
}

void VirtualMachine::ExecuteDIV(RegisterIndex a, int b, int c) {
    // DIV A B C: R(A) := RK(B) / RK(C)
    LuaValue left = GetRK(b);
    LuaValue right = GetRK(c);
    
    std::optional<double> left_num = left.ToNumber();
    std::optional<double> right_num = right.ToNumber();
    
    if (left_num && right_num) {
        double divisor = *right_num;
        if (divisor == 0.0) {
            throw VMExecutionError("Division by zero");
        }
        double result = *left_num / divisor;
        SetRegister(a, LuaValue(result));
    } else {
        throw TypeError("Attempt to perform arithmetic (" + left.TypeName() + 
                       " / " + right.TypeName() + ")");
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
        SetRegister(a, LuaValue(static_cast<double>(value.GetString().length())));
    } else if (value.IsTable()) {
        auto table_ptr = value.GetTable();
        if (table_ptr) {
            Size length = table_ptr->GetArraySize();
            SetRegister(a, LuaValue(static_cast<double>(length)));
        } else {
            SetRegister(a, LuaValue(0.0));
        }
    } else {
        throw TypeError("Attempt to get length of a " + value.TypeName() + " value");
    }
}

void VirtualMachine::ExecuteCONCAT(RegisterIndex a, int b, int c) {
    // CONCAT A B C: R(A) := R(B).. ... ..R(C)
    std::ostringstream result;
    
    for (int i = b; i <= c; i++) {
        LuaValue value = GetRegister(static_cast<RegisterIndex>(i));
        
        if (value.IsString()) {
            result << value.GetString();
        } else if (value.IsNumber()) {
            result << value.GetNumber();
        } else if (value.IsBoolean()) {
            result << (value.GetBoolean() ? "true" : "false");
        } else if (value.IsNil()) {
            result << "nil";
        } else {
            throw TypeError("Attempt to concatenate a " + value.TypeName() + " value");
        }
    }
    
    SetRegister(a, LuaValue(result.str()));
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
        less_than = left.GetNumber() < right.GetNumber();
    } else if (left.IsString() && right.IsString()) {
        less_than = left.GetString() < right.GetString();
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
        less_equal = left.GetNumber() <= right.GetNumber();
    } else if (left.IsString() && right.IsString()) {
        less_equal = left.GetString() <= right.GetString();
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
    
    // 准备参数
    Size param_count = (b == 0) ? (GetStackTop() - GetCurrentBase() - a - 1) : (b - 1);
    
    // 获取函数原型
    const Proto* proto = function.GetFunctionProto();
    if (!proto) {
        throw VMExecutionError("Invalid function proto");
    }
    
    // 创建新的调用帧
    Size new_base = GetCurrentBase() + a;
    PushCallFrame(proto, new_base, param_count);
    
    // 统计信息
    statistics_.function_calls++;
}

void VirtualMachine::ExecuteTAILCALL(RegisterIndex a, int b, int c) {
    // TAILCALL A B C: return R(A)(R(A+1), ... ,R(A+B-1))
    LuaValue function = GetRegister(a);
    
    if (!function.IsFunction()) {
        throw TypeError("Attempt to call a " + function.TypeName() + " value");
    }
    
    // 准备参数
    Size param_count = (b == 0) ? (GetStackTop() - GetCurrentBase() - a - 1) : (b - 1);
    
    // 获取函数原型
    const Proto* proto = function.GetFunctionProto();
    if (!proto) {
        throw VMExecutionError("Invalid function proto");
    }
    
    // 尾调用：复用当前调用帧
    Size current_base = GetCurrentBase();
    
    // 移动参数到正确位置
    for (Size i = 0; i < param_count; ++i) {
        LuaValue param = GetRegister(a + 1 + i);
        SetStack(current_base + i, std::move(param));
    }
    
    // 更新当前函数和指令指针（不增加调用栈深度）
    current_proto_ = proto;
    instruction_pointer_ = 0;
    
    statistics_.function_calls++;
}

void VirtualMachine::ExecuteRETURN(RegisterIndex a, int b) {
    // RETURN A B: return R(A), ... ,R(A+B-2)
    
    // 收集返回值
    std::vector<LuaValue> return_values;
    
    if (b == 0) {
        // 返回从A到栈顶的所有值
        Size stack_top = GetStackTop();
        Size base = GetCurrentBase();
        Size start_index = base + a;
        
        for (Size i = start_index; i < stack_top; i++) {
            return_values.push_back(GetStack(i));
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
    if (!call_stack_->IsEmpty()) {
        // 将返回值放到调用者期望的位置
        // 暂时简化处理：放到当前栈顶
        for (const auto& value : return_values) {
            Push(value);
        }
    } else {
        // 主函数返回，清空栈然后放入返回值
        SetStackTop(0);
        for (const auto& value : return_values) {
            Push(value);
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
    
    double new_init = init.GetNumber() + step.GetNumber();
    SetRegister(a, LuaValue(new_init));
    
    bool continue_loop;
    if (step.GetNumber() > 0) {
        continue_loop = new_init <= limit.GetNumber();
    } else {
        continue_loop = new_init >= limit.GetNumber();
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
    
    double new_init = init.GetNumber() - step.GetNumber();
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
    
    auto table_ptr = table.GetTable();
    if (!table_ptr) {
        throw VMExecutionError("Invalid table in SETLIST");
    }
    
    // FPF = Fields Per Flush = 50 (Lua常量)
    constexpr Size FPF = 50;
    Size base_index = (c == 0) ? 0 : ((c - 1) * FPF);
    
    Size count = (b == 0) ? (GetStackTop() - GetCurrentBase() - a - 1) : b;
    
    for (Size i = 1; i <= count; ++i) {
        LuaValue value = GetRegister(a + i);
        LuaValue index_key = LuaValue(static_cast<double>(base_index + i));
        table_ptr->Set(index_key, std::move(value));
    }
    
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