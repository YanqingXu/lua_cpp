#include "vm.hpp"
#include "object/table.hpp"
#include "object/function.hpp"
#include <iostream>
#include <cmath>

namespace Lua {

VM::VM(Ptr<State> state)
    : m_state(std::move(state))
    , m_pc(0)
    , m_running(false)
{
}

i32 VM::execute(Ptr<Function> function, i32 nargs, i32 nresults) {
    if (!function || !function->prototype()) {
        m_state->error("Cannot execute null function");
        return 0;
    }

    try {
        // 保存当前执行状态
        bool wasRunning = m_running;
        m_running = true;
        
        // 放置函数和参数到栈上
        i32 funcIdx = m_state->getTop() - nargs;
        
        // 将函数放到调用栈中
        pushCallInfo(function, nargs, -1);
        
        // 设置程序计数器
        m_pc = 0;
        
        // 执行指令，直到函数返回
        while (m_running && !m_callStack.empty()) {
            // 获取当前要执行的指令
            Ptr<FunctionProto> proto = getCurrentProto();
            if (!proto || m_pc >= proto->getCode().size()) {
                m_state->error("Invalid program counter or function prototype");
                break;
            }
            
            // 获取指令并执行
            Instruction instr = proto->getCode()[m_pc++];
            executeInstruction(instr);
        }
        
        // 恢复之前的状态
        m_running = wasRunning;
        
        // 返回结果数量
        return std::min(nresults, m_state->getTop() - funcIdx);
    }
    catch (const LuaException& e) {
        std::cerr << "Lua runtime error: " << e.what() << std::endl;
        m_running = false;
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "C++ exception: " << e.what() << std::endl;
        m_running = false;
        return 0;
    }
}

void VM::executeInstruction(const Instruction& instr) {
    OpCode op = instr.getOpCode();
    
    switch (op) {
        // 算术操作
        case OpCode::Add:
        case OpCode::Sub:
        case OpCode::Mul:
        case OpCode::Div:
        case OpCode::Mod:
        case OpCode::Pow:
        case OpCode::Concat:
            executeArithmetic(instr);
            break;
            
        // 比较操作
        case OpCode::Eq:
        case OpCode::Lt:
        case OpCode::Le:
            executeComparison(instr);
            break;
            
        // 加载操作
        case OpCode::LoadNil:
        case OpCode::LoadTrue:
        case OpCode::LoadFalse:
        case OpCode::LoadK:
            executeLoadConstant(instr);
            break;
            
        // 跳转操作
        case OpCode::Jump:
        case OpCode::JumpIfTrue:
        case OpCode::JumpIfFalse:
            executeJump(instr);
            break;
            
        // 调用操作
        case OpCode::Call:
        case OpCode::TailCall:
            executeCall(instr);
            break;
            
        // 返回操作
        case OpCode::Return:
            executeReturn(instr);
            break;
            
        // 表操作
        case OpCode::NewTable:
        case OpCode::GetTable:
        case OpCode::SetTable:
        case OpCode::GetField:
        case OpCode::SetField:
            executeTableOperations(instr);
            break;
            
        // Upvalue操作
        case OpCode::GetUpval:
        case OpCode::SetUpval:
        case OpCode::GetGlobal:
        case OpCode::SetGlobal:
            executeUpvalueOperations(instr);
            break;
            
        // 其他操作
        default:
            executeOtherOperations(instr);
            break;
    }
}

void VM::executeArithmetic(const Instruction& instr) {
    OpCode op = instr.getOpCode();
    u8 a = instr.getA();
    u8 b = instr.getB();
    u8 c = instr.getC();
    
    Value& ra = getRegister(a);
    const Value& rb = getRegister(b);
    const Value& rc = getRegister(c);
    
    switch (op) {
        case OpCode::Add:
            if (rb.type() == Value::Type::Number && rc.type() == Value::Type::Number) {
                ra = Value(rb.asNumber() + rc.asNumber());
            } else {
                // TODO: 处理元方法
                m_state->error("Attempt to add non-numeric values");
            }
            break;
            
        case OpCode::Sub:
            if (rb.type() == Value::Type::Number && rc.type() == Value::Type::Number) {
                ra = Value(rb.asNumber() - rc.asNumber());
            } else {
                // TODO: 处理元方法
                m_state->error("Attempt to subtract non-numeric values");
            }
            break;
            
        case OpCode::Mul:
            if (rb.type() == Value::Type::Number && rc.type() == Value::Type::Number) {
                ra = Value(rb.asNumber() * rc.asNumber());
            } else {
                // TODO: 处理元方法
                m_state->error("Attempt to multiply non-numeric values");
            }
            break;
            
        case OpCode::Div:
            if (rb.type() == Value::Type::Number && rc.type() == Value::Type::Number) {
                if (rc.asNumber() == 0.0) {
                    m_state->error("Division by zero");
                } else {
                    ra = Value(rb.asNumber() / rc.asNumber());
                }
            } else {
                // TODO: 处理元方法
                m_state->error("Attempt to divide non-numeric values");
            }
            break;
            
        case OpCode::Mod:
            if (rb.type() == Value::Type::Number && rc.type() == Value::Type::Number) {
                double b_val = rb.asNumber();
                double c_val = rc.asNumber();
                ra = Value(b_val - floor(b_val / c_val) * c_val);
            } else {
                // TODO: 处理元方法
                m_state->error("Attempt to perform modulo on non-numeric values");
            }
            break;
            
        case OpCode::Pow:
            if (rb.type() == Value::Type::Number && rc.type() == Value::Type::Number) {
                ra = Value(pow(rb.asNumber(), rc.asNumber()));
            } else {
                // TODO: 处理元方法
                m_state->error("Attempt to perform power operation on non-numeric values");
            }
            break;
            
        case OpCode::Concat:
            // TODO: 实现字符串连接
            break;
            
        default:
            break;
    }
}

void VM::executeComparison(const Instruction& instr) {
    OpCode op = instr.getOpCode();
    u8 a = instr.getA();
    u8 b = instr.getB();
    u8 c = instr.getC();
    
    const Value& rb = getRegister(b);
    const Value& rc = getRegister(c);
    
    bool result = false;
    
    switch (op) {
        case OpCode::Eq:
            result = (rb == rc);
            break;
            
        case OpCode::Lt:
            if (rb.type() == Value::Type::Number && rc.type() == Value::Type::Number) {
                result = (rb.asNumber() < rc.asNumber());
            } else {
                // TODO: 处理元方法
                m_state->error("Attempt to compare non-numeric values with <");
            }
            break;
            
        case OpCode::Le:
            if (rb.type() == Value::Type::Number && rc.type() == Value::Type::Number) {
                result = (rb.asNumber() <= rc.asNumber());
            } else {
                // TODO: 处理元方法
                m_state->error("Attempt to compare non-numeric values with <=");
            }
            break;
            
        default:
            break;
    }
    
    // 判断是否需要跳过下一条指令
    if (result == static_cast<bool>(a)) {
        m_pc++;  // 跳过下一条指令
    }
}

void VM::executeLoadConstant(const Instruction& instr) {
    OpCode op = instr.getOpCode();
    u8 a = instr.getA();
    
    switch (op) {
        case OpCode::LoadNil:
            getRegister(a) = Value();  // nil
            break;
            
        case OpCode::LoadTrue:
            getRegister(a) = Value(true);
            break;
            
        case OpCode::LoadFalse:
            getRegister(a) = Value(false);
            break;
            
        case OpCode::LoadK:
            getRegister(a) = getConstant(instr.getBx());
            break;
            
        default:
            break;
    }
}

void VM::executeJump(const Instruction& instr) {
    OpCode op = instr.getOpCode();
    u8 a = instr.getA();
    i16 sbx = instr.getSBx();
    
    switch (op) {
        case OpCode::Jump:
            m_pc += sbx;
            break;
            
        case OpCode::JumpIfTrue:
            if (getRegister(a).toBoolean()) {
                m_pc += sbx;
            }
            break;
            
        case OpCode::JumpIfFalse:
            if (!getRegister(a).toBoolean()) {
                m_pc += sbx;
            }
            break;
            
        default:
            break;
    }
}

void VM::executeCall(const Instruction& instr) {
    OpCode op = instr.getOpCode();
    u8 a = instr.getA();
    u8 b = instr.getB();
    u8 c = instr.getC();
    
    i32 nargs = b - 1;  // B参数指定参数个数 (包括函数本身)
    i32 nresults = c - 1;  // C参数指定期望的返回值个数
    
    Value& func = getRegister(a);
    
    if (func.type() != Value::Type::Function) {
        m_state->error("Attempt to call a non-function value");
        return;
    }
    
    Ptr<Function> function = func.asFunction();
    
    if (op == OpCode::TailCall) {
        // 尾调用优化：复用当前调用帧
        if (m_callStack.empty()) {
            m_state->error("Tail call with empty call stack");
            return;
        }
        
        // 保存返回PC
        i32 returnPC = m_callStack.back().returnPC;
        
        // 弹出当前调用帧
        popCallInfo();
        
        // 推入新的调用帧，但保留相同的返回PC
        pushCallInfo(function, nargs, returnPC);
    } else {
        // 正常调用：创建新的调用帧
        pushCallInfo(function, nargs, m_pc);
    }
    
    // 重置PC为0，以便开始执行新函数
    m_pc = 0;
}

void VM::executeReturn(const Instruction& instr) {
    u8 a = instr.getA();
    u8 b = instr.getB();
    
    i32 firstResult = a;
    i32 nresults = b - 1;  // B参数指定要返回的值的数量
    
    if (m_callStack.empty()) {
        m_state->error("Return with empty call stack");
        return;
    }
    
    // 弹出当前调用帧
    i32 returnPC = m_callStack.back().returnPC;
    popCallInfo();
    
    // 如果这是最后一个调用帧，我们已经完成执行
    if (m_callStack.empty()) {
        m_running = false;
        return;
    }
    
    // 返回到调用函数
    m_pc = returnPC;
    
    // TODO: 处理返回值
}

void VM::executeTableOperations(const Instruction& instr) {
    OpCode op = instr.getOpCode();
    u8 a = instr.getA();
    u8 b = instr.getB();
    u8 c = instr.getC();
    
    switch (op) {
        case OpCode::NewTable:
            getRegister(a) = Value(std::make_shared<Table>());
            break;
            
        case OpCode::GetTable: {
            const Value& table = getRegister(b);
            const Value& key = getRegister(c);
            
            if (table.type() != Value::Type::Table) {
                m_state->error("Attempt to index a non-table value");
                return;
            }
            
            Ptr<Table> tableObj = table.asTable();
            getRegister(a) = tableObj->get(key);
            break;
        }
            
        case OpCode::SetTable: {
            const Value& table = getRegister(a);
            const Value& key = getRegister(b);
            const Value& value = getRegister(c);
            
            if (table.type() != Value::Type::Table) {
                m_state->error("Attempt to index a non-table value");
                return;
            }
            
            Ptr<Table> tableObj = table.asTable();
            tableObj->set(key, value);
            break;
        }
            
        case OpCode::GetField: {
            const Value& table = getRegister(b);
            const Value& key = getConstant(c); // C是常量索引
            
            if (table.type() != Value::Type::Table) {
                m_state->error("Attempt to index a non-table value");
                return;
            }
            
            Ptr<Table> tableObj = table.asTable();
            getRegister(a) = tableObj->get(key);
            break;
        }
            
        case OpCode::SetField: {
            const Value& table = getRegister(a);
            const Value& key = getConstant(b); // B是常量索引
            const Value& value = getRegister(c);
            
            if (table.type() != Value::Type::Table) {
                m_state->error("Attempt to index a non-table value");
                return;
            }
            
            Ptr<Table> tableObj = table.asTable();
            tableObj->set(key, value);
            break;
        }
            
        default:
            break;
    }
}

void VM::executeUpvalueOperations(const Instruction& instr) {
    // TODO: 实现upvalue操作
}

void VM::executeOtherOperations(const Instruction& instr) {
    OpCode op = instr.getOpCode();
    u8 a = instr.getA();
    
    switch (op) {
        case OpCode::Move: {
            u8 b = instr.getB();
            getRegister(a) = getRegister(b);
            break;
        }
            
        case OpCode::Len: {
            u8 b = instr.getB();
            const Value& val = getRegister(b);
            
            if (val.type() == Value::Type::String) {
                getRegister(a) = Value(static_cast<double>(val.asString().size()));
            } else if (val.type() == Value::Type::Table) {
                getRegister(a) = Value(static_cast<double>(val.asTable()->length()));
            } else {
                // TODO: 处理元方法
                m_state->error("Attempt to get length of a non-string, non-table value");
            }
            break;
        }
            
        case OpCode::Not: {
            u8 b = instr.getB();
            getRegister(a) = Value(!getRegister(b).asBoolean());
            break;
        }
            
        case OpCode::Pop: {
            // 弹出A个值
            m_state->pop(a);
            break;
        }
            
        // TODO: 处理更多指令
            
        default:
            m_state->error("Unsupported operation: " + std::to_string(static_cast<i32>(op)));
            break;
    }
}

Ptr<Function> VM::getCurrentFunction() const {
    if (m_callStack.empty()) {
        return nullptr;
    }
    return m_callStack.back().function;
}

Ptr<FunctionProto> VM::getCurrentProto() const {
    Ptr<Function> func = getCurrentFunction();
    if (!func) {
        return nullptr;
    }
    return func->getProto();
}

Value& VM::getRegister(i32 reg) {
    if (m_callStack.empty()) {
        m_state->error("Access register with empty call stack");
        static Value dummy;
        return dummy;
    }
    
    i32 baseReg = m_callStack.back().baseReg;
    return m_state->peek(baseReg + reg);
}

Value VM::getConstant(i32 idx) const {
    Ptr<FunctionProto> proto = getCurrentProto();
    if (!proto || idx < 0 || idx >= proto->getConstants().size()) {
        m_state->error("Invalid constant index");
        return Value(); // nil
    }
    
    return proto->getConstants()[idx];
}

CallInfo& VM::pushCallInfo(Ptr<Function> func, i32 nargs, i32 returnPC) {
    // 基址是当前栈顶减去参数数量
    i32 baseReg = m_state->getTop() - nargs - 1;
    
    m_callStack.emplace_back(func, baseReg, returnPC);
    return m_callStack.back();
}

void VM::popCallInfo() {
    if (!m_callStack.empty()) {
        m_callStack.pop_back();
    }
}

}