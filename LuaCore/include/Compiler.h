#pragma once

#include "AST.h"
#include "Value.h"
#include "types.h"
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

namespace LuaCore {

/**
 * @brief 表示编译过程中的局部变量
 */
struct LocalVar {
    Str name;    // 变量名
    i32 scopeDepth; // 定义该变量的作用域深度
    bool isCaptured; // 是否被内层函数捕获
    i32 slot;       // 变量在栈上的位置
};

/**
 * @brief 表示Upvalue（被内部函数捕获的外部变量）
 */
struct Upvalue {
    u8 index;       // 变量在外层函数局部变量表或upvalue表中的索引
    bool isLocal;   // 是否是直接外层函数的局部变量
};

/**
 * @brief Lua字节码操作码
 */
enum class OpCode : u8 {
    // 加载常量和基本值
    LoadNil,        // 加载nil值到寄存器
    LoadTrue,       // 加载true值到寄存器
    LoadFalse,      // 加载false值到寄存器
    LoadK,          // 从常量表加载常量到寄存器
    
    // 表操作
    NewTable,       // 创建新表
    GetTable,       // 从表中获取值 (R(A) := R(B)[R(C)])
    SetTable,       // 设置表的值 (R(A)[R(B)] := R(C))
    GetField,       // 从表中获取字段 (R(A) := R(B)[K(C)])
    SetField,       // 设置表字段  (R(A)[K(B)] := R(C))
    
    // 算术和比较操作
    Add,            // R(A) := R(B) + R(C)
    Sub,            // R(A) := R(B) - R(C)
    Mul,            // R(A) := R(B) * R(C)
    Div,            // R(A) := R(B) / R(C)
    Mod,            // R(A) := R(B) % R(C)
    Pow,            // R(A) := R(B) ^ R(C)
    Concat,         // R(A) := R(B)..R(C)
    
    // 一元操作
    Neg,            // R(A) := -R(B)
    Not,            // R(A) := not R(B)
    Len,            // R(A) := #R(B)
    
    // 比较操作
    Eq,             // if ((R(B) == R(C)) ~= A) then PC++
    Lt,             // if ((R(B) < R(C)) ~= A) then PC++
    Le,             // if ((R(B) <= R(C)) ~= A) then PC++
    
    // 逻辑操作
    Test,           // if not (R(A) <=> C) then PC++
    TestSet,        // if (R(B) <=> C) then R(A) := R(B) else PC++
    
    // 控制流
    Jump,           // PC += sBx
    JumpIfTrue,     // if R(A) then PC += sBx
    JumpIfFalse,    // if not R(A) then PC += sBx
    ForLoop,        // R(A)+=R(A+2); if R(A) <?= R(A+1) then { PC+=sBx; R(A+3)=R(A) }
    ForPrep,        // R(A)-=R(A+2); PC+=sBx
    
    // 函数调用和返回
    Call,           // R(A), ... ,R(A+C-2) := R(A)(R(A+1), ... ,R(A+B-1))
    TailCall,       // return R(A)(R(A+1), ... ,R(A+B-1))
    Return,         // return R(A), ... ,R(A+B-2)
    
    // 闭包和Upvalue
    Closure,        // R(A) := closure(KPROTO[Bx])
    GetUpvalue,     // R(A) := UpValue[B]
    SetUpvalue,     // UpValue[B] := R(A)
    Close,          // 关闭位于R(A)及其之后的所有upvalue
    
    // 杂项
    Move,           // R(A) := R(B)
    Self,           // R(A+1) := R(B); R(A) := R(B)[R(C)]
    
    // 变长参数处理
    VarArg,         // R(A), R(A+1), ..., R(A+B-2) = vararg
};

/**
 * @brief 字节码指令结构
 */
struct Instruction {
    u32 code;   // 32位指令编码
    
    // 指令编码格式:
    // +---+---+---+---+
    // |  B  |  C  |  A  |
    // +---+---+---+---+
    // |  Bx   |  A  |
    // +---+---+---+---+
    // |   sBx   |  A  |
    // +---+---+---+---+
    // |      Ax       |
    // +---+---+---+---+
    
    // 获取不同字段的方法
    u8 getA() const { return (code >> 6) & 0xFF; }
    u8 getB() const { return (code >> 23) & 0x1FF; }
    u8 getC() const { return (code >> 14) & 0x1FF; }
    u16 getBx() const { return (code >> 14) & 0x3FFFF; }
    i16 getSBx() const { return static_cast<i16>(getBx()) - 131071; } // 有符号版本, max_unsigned/2
    u32 getAx() const { return code >> 6; }
    
    // 创建不同格式指令的静态方法
    static Instruction create(OpCode op, u8 a, u8 b, u8 c) {
        return { static_cast<u32>(static_cast<u8>(op) | (a << 6) | (b << 23) | (c << 14)) };
    }
    
    static Instruction createABC(OpCode op, u8 a, u8 b, u8 c) {
        return create(op, a, b, c);
    }
    
    static Instruction createABx(OpCode op, u8 a, u16 bx) {
        return { static_cast<u32>(static_cast<u8>(op) | (a << 6) | (bx << 14)) };
    }
    
    static Instruction createASBx(OpCode op, u8 a, i16 sbx) {
        return createABx(op, a, static_cast<u16>(sbx + 131071)); // 转换为无符号值
    }
    
    static Instruction createAx(OpCode op, u32 ax) {
        return { static_cast<u32>(static_cast<u8>(op) | (ax << 6)) };
    }
    
    OpCode getOpCode() const {
        return static_cast<OpCode>(code & 0x3F); // 低6位是操作码
    }
};

/**
 * @brief 表示一个函数原型
 */
class FunctionProto {
public:
    FunctionProto(const Str& name, i32 numParams, bool isVararg)
        : m_name(name), m_numParams(numParams), m_isVararg(isVararg) {}
    
    // 添加常量到常量表
    usize addConstant(const Value& value);
    
    // 获取常量表
    const Vec<Value>& getConstants() const { return m_constants; }
    
    // 添加局部变量
    void addLocalVar(const Str& name, i32 scopeDepth);
    
    // 添加upvalue
    i32 addUpvalue(u8 index, bool isLocal);
    
    // 添加指令
    usize addInstruction(const Instruction& instruction);
    
    // 获取指令
    Vec<Instruction>& getCode() { return m_code; }
    const Vec<Instruction>& getCode() const { return m_code; }
    
    // 添加子函数原型
    void addProto(Ptr<FunctionProto> proto);
    
    // 获取子函数原型
    const Vec<Ptr<FunctionProto>>& getProtos() const { return m_protos; }
    
    // 设置行号信息
    void setLineInfo(usize instructionIndex, i32 line);
    
    // 获取行号信息
    i32 getLine(usize instructionIndex) const;
    
    // 获取函数原型的基本信息
    const Str& getName() const { return m_name; }
    i32 getNumParams() const { return m_numParams; }
    bool isVararg() const { return m_isVararg; }
    
    // 获取局部变量表和upvalue表
    Vec<LocalVar>& getLocalVars() { return m_localVars; }
    const Vec<LocalVar>& getLocalVars() const { return m_localVars; }
    const Vec<Upvalue>& getUpvalues() const { return m_upvalues; }
    
    i32 getMaxStackSize() const { return m_maxStackSize; }
    
private:
    Str m_name;                // 函数名
    i32 m_numParams;           // 参数数量
    bool m_isVararg;           // 是否支持变长参数
    Vec<Value> m_constants;    // 常量表
    Vec<Instruction> m_code;   // 指令列表
    Vec<i32> m_lineInfo;       // 行号信息
    Vec<LocalVar> m_localVars; // 局部变量表
    Vec<Upvalue> m_upvalues;   // upvalue表
    Vec<Ptr<FunctionProto>> m_protos; // 子函数原型
    i32 m_maxStackSize = 0;    // 最大栈大小
};

/**
 * @brief 编译器类，负责将AST转换为字节码
 */
class Compiler {
public:
    /**
     * 编译代码
     * @param ast 抽象语法树
     * @param source 源代码（用于错误报告）
     * @return 编译后的函数原型
     */
    Ptr<FunctionProto> compile(Ptr<Block> ast, const Str& source);
    
private:
    struct CompileState {
        Ptr<FunctionProto> proto;                  // 当前函数原型
        HashMap<Str, i32> locals;               // 局部变量名到索引的映射
        Vec<Upvalue> upvalues;                     // upvalue表
        i32 scopeDepth = 0;                        // 当前作用域深度
        i32 localCount = 0;                        // 局部变量数量
        i32 stackSize = 0;                         // 当前栈大小
        CompileState* enclosing = nullptr;         // 外层编译状态
    };
    
    CompileState* m_current = nullptr;    // 当前编译状态
    Str m_source;                      // 源代码
    
    // 作用域管理
    void beginScope();
    void endScope();
    
    // 局部变量管理
    i32 addLocal(const Str& name);
    i32 resolveLocal(CompileState* state, const Str& name);
    i32 resolveUpvalue(CompileState* state, const Str& name);
    
    // 表达式编译
    void compileExpression(Ptr<Expression> expr, i32 reg);
    void compileBinaryExpr(Ptr<BinaryExpr> expr, i32 reg);
    void compileUnaryExpr(Ptr<UnaryExpr> expr, i32 reg);
    void compileLiteralExpr(Ptr<LiteralExpr> expr, i32 reg);
    void compileVariableExpr(Ptr<VariableExpr> expr, i32 reg);
    void compileTableAccessExpr(Ptr<TableAccessExpr> expr, i32 reg);
    void compileFieldAccessExpr(Ptr<FieldAccessExpr> expr, i32 reg);
    void compileFunctionCallExpr(Ptr<FunctionCallExpr> expr, i32 reg);
    void compileTableConstructorExpr(Ptr<TableConstructorExpr> expr, i32 reg);
    void compileFunctionDefExpr(Ptr<FunctionDefExpr> expr, i32 reg);
    
    // 语句编译
    void compileStatement(Ptr<Statement> stmt);
    void compileBlock(Ptr<Block> block);
    void compileAssignmentStmt(Ptr<AssignmentStmt> stmt);
    void compileLocalVarDeclStmt(Ptr<LocalVarDeclStmt> stmt);
    void compileFunctionCallStmt(Ptr<FunctionCallStmt> stmt);
    void compileIfStmt(Ptr<IfStmt> stmt);
    void compileWhileStmt(Ptr<WhileStmt> stmt);
    void compileDoStmt(Ptr<DoStmt> stmt);
    void compileForStmt(Ptr<NumericForStmt> stmt);
    void compileGenericForStmt(Ptr<GenericForStmt> stmt);
    void compileRepeatStmt(Ptr<RepeatStmt> stmt);
    void compileFunctionDeclStmt(Ptr<FunctionDeclStmt> stmt);
    void compileReturnStmt(Ptr<ReturnStmt> stmt);
    void compileBreakStmt(Ptr<BreakStmt> stmt);
    
    // 指令生成
    usize emitInstruction(const Instruction& instruction, i32 line);
    usize emitABC(OpCode op, u8 a, u8 b, u8 c, i32 line);
    usize emitABx(OpCode op, u8 a, u16 bx, i32 line);
    usize emitASBx(OpCode op, u8 a, i16 sbx, i32 line);
    usize emitAx(OpCode op, u32 ax, i32 line);
    
    // 跳转指令处理
    usize emitJump(OpCode op, i32 line);
    void patchJump(usize jumpInstr, usize target);
    
    // 常量处理
    i32 addConstant(const Value& value);
    i32 addStringConstant(const Str& str);
};

} // namespace LuaCore
