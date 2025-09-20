#pragma once

#include "core/common.h"
#include "core/lua_value.h"
#include <vector>
#include <cstdint>
#include <memory>
#include <optional>

namespace lua_cpp {

/* ========================================================================== */
/* Lua 5.1.5 指令格式 */
/* ========================================================================== */

/**
 * @brief Lua指令类型 - 32位指令
 * 
 * Lua 5.1.5 使用32位指令，包含三种格式：
 * - iABC: 6位操作码 + 8位A + 9位B + 9位C
 * - iABx: 6位操作码 + 8位A + 18位Bx
 * - iAsBx: 6位操作码 + 8位A + 18位sBx (有符号)
 */
using Instruction = uint32_t;

/**
 * @brief 指令格式枚举
 */
enum class InstructionMode {
    iABC,   // A(8) B(9) C(9)
    iABx,   // A(8) Bx(18)
    iAsBx   // A(8) sBx(18) - signed
};

/**
 * @brief Lua 5.1.5 操作码定义
 * 
 * 按照Lua 5.1.5官方实现的操作码顺序和定义
 */
enum class OpCode : uint8_t {
    // 数据移动指令
    MOVE = 0,       // R(A) := R(B)
    LOADK,          // R(A) := Kst(Bx)
    LOADBOOL,       // R(A) := (Bool)B; if (C) pc++
    LOADNIL,        // R(A) := ... := R(B) := nil
    
    // 全局变量指令
    GETUPVAL,       // R(A) := UpValue[B]
    GETGLOBAL,      // R(A) := Gbl[Kst(Bx)]
    GETTABLE,       // R(A) := R(B)[RK(C)]
    
    SETGLOBAL,      // Gbl[Kst(Bx)] := R(A)
    SETUPVAL,       // UpValue[B] := R(A)
    SETTABLE,       // R(A)[RK(B)] := RK(C)
    
    // 表构造指令
    NEWTABLE,       // R(A) := {} (size = B,C)
    
    // 算术和位运算指令
    SELF,           // R(A+1) := R(B); R(A) := R(B)[RK(C)]
    ADD,            // R(A) := RK(B) + RK(C)
    SUB,            // R(A) := RK(B) - RK(C)
    MUL,            // R(A) := RK(B) * RK(C)
    DIV,            // R(A) := RK(B) / RK(C)
    MOD,            // R(A) := RK(B) % RK(C)
    POW,            // R(A) := RK(B) ^ RK(C)
    UNM,            // R(A) := -R(B)
    NOT,            // R(A) := not R(B)
    LEN,            // R(A) := length of R(B)
    
    // 字符串连接
    CONCAT,         // R(A) := R(B).. ... ..R(C)
    
    // 跳转指令
    JMP,            // pc+=sBx
    
    // 比较指令
    EQ,             // if ((RK(B) == RK(C)) ~= A) then pc++
    LT,             // if ((RK(B) <  RK(C)) ~= A) then pc++
    LE,             // if ((RK(B) <= RK(C)) ~= A) then pc++
    
    // 测试指令
    TEST,           // if not (R(A) <=> C) then pc++
    TESTSET,        // if (R(B) <=> C) then R(A) := R(B) else pc++
    
    // 函数调用指令
    CALL,           // R(A), ... ,R(A+C-2) := R(A)(R(A+1), ... ,R(A+B-1))
    TAILCALL,       // return R(A)(R(A+1), ... ,R(A+B-1))
    RETURN,         // return R(A), ... ,R(A+B-2)
    
    // 循环指令
    FORLOOP,        // R(A)+=R(A+2); if R(A) <?= R(A+1) then { pc+=sBx; R(A+3)=R(A) }
    FORPREP,        // R(A)-=R(A+2); pc+=sBx
    
    // 泛型for循环
    TFORLOOP,       // if R(A+1) ~= nil then { R(A)=R(A+1); pc += sBx }
    
    // 表初始化
    SETLIST,        // R(A)[(C-1)*FPF+i] := R(A+i), 1 <= i <= B
    
    // 闭包创建
    CLOSE,          // close all variables in the stack up to (>=) R(A)
    CLOSURE,        // R(A) := closure(KPROTO[Bx], R(A), ... ,R(A+n))
    
    // 可变参数
    VARARG,         // R(A), R(A+1), ..., R(A+B-1) = vararg
    
    // 操作码数量
    NUM_OPCODES
};

/**
 * @brief 操作码属性结构
 */
struct OpCodeInfo {
    const char* name;               // 操作码名称
    InstructionMode mode;           // 指令格式
    bool test_flag;                 // 是否是测试指令
    bool set_register_a;            // 是否设置寄存器A
    
    constexpr OpCodeInfo(const char* n, InstructionMode m, bool test = false, bool set_a = true)
        : name(n), mode(m), test_flag(test), set_register_a(set_a) {}
};

/**
 * @brief 操作码信息表
 */
extern const OpCodeInfo OPCODE_INFO[static_cast<int>(OpCode::NUM_OPCODES)];

/* ========================================================================== */
/* 指令字段访问函数 */
/* ========================================================================== */

// 指令字段位移和掩码常量
constexpr int SIZE_C = 9;
constexpr int SIZE_B = 9;
constexpr int SIZE_Bx = (SIZE_C + SIZE_B);
constexpr int SIZE_A = 8;
constexpr int SIZE_OP = 6;

constexpr int POS_OP = 0;
constexpr int POS_A = (POS_OP + SIZE_OP);
constexpr int POS_C = (POS_A + SIZE_A);
constexpr int POS_B = (POS_C + SIZE_C);
constexpr int POS_Bx = POS_C;

// 字段掩码
constexpr uint32_t MASK1(int n, int p) { return ((~((~0u) << n)) << p); }
constexpr uint32_t MASK0(int n, int p) { return (~MASK1(n, p)); }

constexpr uint32_t MAXARG_Bx = ((1u << SIZE_Bx) - 1);
constexpr uint32_t MAXARG_sBx = (MAXARG_Bx >> 1);

constexpr uint32_t MAXARG_A = ((1u << SIZE_A) - 1);
constexpr uint32_t MAXARG_B = ((1u << SIZE_B) - 1);
constexpr uint32_t MAXARG_C = ((1u << SIZE_C) - 1);

// 偏移常量
constexpr int MAXARG_sBx_OFFSET = (MAXARG_sBx);

/**
 * @brief 获取指令的操作码
 */
inline OpCode GetOpCode(Instruction i) {
    return static_cast<OpCode>((i >> POS_OP) & MASK1(SIZE_OP, 0));
}

/**
 * @brief 设置指令的操作码
 */
inline Instruction SetOpCode(Instruction i, OpCode o) {
    return (i & MASK0(SIZE_OP, POS_OP)) | ((static_cast<uint32_t>(o) << POS_OP) & MASK1(SIZE_OP, POS_OP));
}

/**
 * @brief 获取指令的A字段
 */
inline int GetArgA(Instruction i) {
    return static_cast<int>((i >> POS_A) & MASK1(SIZE_A, 0));
}

/**
 * @brief 设置指令的A字段
 */
inline Instruction SetArgA(Instruction i, int u) {
    return (i & MASK0(SIZE_A, POS_A)) | ((static_cast<uint32_t>(u) << POS_A) & MASK1(SIZE_A, POS_A));
}

/**
 * @brief 获取指令的B字段
 */
inline int GetArgB(Instruction i) {
    return static_cast<int>((i >> POS_B) & MASK1(SIZE_B, 0));
}

/**
 * @brief 设置指令的B字段
 */
inline Instruction SetArgB(Instruction i, int u) {
    return (i & MASK0(SIZE_B, POS_B)) | ((static_cast<uint32_t>(u) << POS_B) & MASK1(SIZE_B, POS_B));
}

/**
 * @brief 获取指令的C字段
 */
inline int GetArgC(Instruction i) {
    return static_cast<int>((i >> POS_C) & MASK1(SIZE_C, 0));
}

/**
 * @brief 设置指令的C字段
 */
inline Instruction SetArgC(Instruction i, int u) {
    return (i & MASK0(SIZE_C, POS_C)) | ((static_cast<uint32_t>(u) << POS_C) & MASK1(SIZE_C, POS_C));
}

/**
 * @brief 获取指令的Bx字段
 */
inline int GetArgBx(Instruction i) {
    return static_cast<int>((i >> POS_Bx) & MASK1(SIZE_Bx, 0));
}

/**
 * @brief 设置指令的Bx字段
 */
inline Instruction SetArgBx(Instruction i, int u) {
    return (i & MASK0(SIZE_Bx, POS_Bx)) | ((static_cast<uint32_t>(u) << POS_Bx) & MASK1(SIZE_Bx, POS_Bx));
}

/**
 * @brief 获取指令的sBx字段
 */
inline int GetArgsBx(Instruction i) {
    return GetArgBx(i) - MAXARG_sBx_OFFSET;
}

/**
 * @brief 设置指令的sBx字段
 */
inline Instruction SetArgsBx(Instruction i, int u) {
    return SetArgBx(i, u + MAXARG_sBx_OFFSET);
}

/**
 * @brief 创建ABC格式指令
 */
inline Instruction CreateABC(OpCode o, int a, int b, int c) {
    return (static_cast<uint32_t>(o) << POS_OP) |
           (static_cast<uint32_t>(a) << POS_A) |
           (static_cast<uint32_t>(b) << POS_B) |
           (static_cast<uint32_t>(c) << POS_C);
}

/**
 * @brief 创建ABx格式指令
 */
inline Instruction CreateABx(OpCode o, int a, int bx) {
    return (static_cast<uint32_t>(o) << POS_OP) |
           (static_cast<uint32_t>(a) << POS_A) |
           (static_cast<uint32_t>(bx) << POS_Bx);
}

/**
 * @brief 创建AsBx格式指令
 */
inline Instruction CreateAsBx(OpCode o, int a, int sbx) {
    return CreateABx(o, a, sbx + MAXARG_sBx_OFFSET);
}

/* ========================================================================== */
/* RK值编码 */
/* ========================================================================== */

/**
 * @brief RK值标识位 - 用于区分寄存器和常量
 */
constexpr int BITRK = (1 << (SIZE_B - 1));

/**
 * @brief 检查RK值是否是常量
 */
inline bool IsConstant(int rk) {
    return (rk & BITRK) != 0;
}

/**
 * @brief 从RK值中提取常量索引
 */
inline int RKToConstantIndex(int rk) {
    return rk & ~BITRK;
}

/**
 * @brief 将常量索引编码为RK值
 */
inline int ConstantIndexToRK(int k) {
    return k | BITRK;
}

/**
 * @brief 从RK值中提取寄存器索引
 */
inline int RKToRegisterIndex(int rk) {
    return rk;
}

/**
 * @brief 将寄存器索引编码为RK值
 */
inline int RegisterIndexToRK(int r) {
    return r;
}

/* ========================================================================== */
/* 函数原型和常量管理 */
/* ========================================================================== */

/**
 * @brief 上值描述符类型
 */
enum class UpvalueType {
    Local,      // 来自外层函数的局部变量
    Upvalue     // 来自外层函数的上值
};

/**
 * @brief 上值描述符
 */
struct UpvalueDesc {
    UpvalueType type;
    RegisterIndex index;
    
    UpvalueDesc(UpvalueType t, RegisterIndex idx) : type(t), index(idx) {}
};

/**
 * @brief 局部变量信息
 */
struct LocalVarInfo {
    std::string name;           // 变量名
    RegisterIndex register_idx; // 寄存器索引
    Size start_pc;             // 作用域开始PC
    Size end_pc;               // 作用域结束PC
    
    LocalVarInfo(const std::string& n, RegisterIndex reg, Size start = 0, Size end = 0)
        : name(n), register_idx(reg), start_pc(start), end_pc(end) {}
};

/**
 * @brief 调试行信息
 */
struct LineInfo {
    Size pc;        // 指令位置
    int line;       // 源代码行号
    
    LineInfo(Size p, int l) : pc(p), line(l) {}
};

/**
 * @brief 函数原型类 - 存储编译后的函数信息
 * 
 * Proto类包含Lua函数的所有编译时信息：
 * - 指令序列
 * - 常量表
 * - 子函数原型
 * - 上值描述符
 * - 调试信息
 */
class Proto {
public:
    /**
     * @brief 构造函数
     * @param source_name 源代码文件名
     * @param line_defined 函数定义行号
     */
    explicit Proto(const std::string& source_name = "", int line_defined = 0);
    
    /**
     * @brief 析构函数
     */
    ~Proto() = default;
    
    // 禁用拷贝，允许移动
    Proto(const Proto&) = delete;
    Proto& operator=(const Proto&) = delete;
    Proto(Proto&&) = default;
    Proto& operator=(Proto&&) = default;
    
    /* ====================================================================== */
    /* 指令管理 */
    /* ====================================================================== */
    
    /**
     * @brief 添加指令
     * @param instruction 指令
     * @param line 源代码行号
     * @return 指令在代码中的位置
     */
    Size AddInstruction(Instruction instruction, int line = 0);
    
    /**
     * @brief 获取指令序列
     */
    const std::vector<Instruction>& GetCode() const { return code_; }
    
    /**
     * @brief 获取可修改的指令序列
     */
    std::vector<Instruction>& GetCode() { return code_; }
    
    /**
     * @brief 获取指定位置的指令
     */
    Instruction GetInstruction(Size pc) const;
    
    /**
     * @brief 设置指定位置的指令
     */
    void SetInstruction(Size pc, Instruction instruction);
    
    /**
     * @brief 获取代码大小
     */
    Size GetCodeSize() const { return code_.size(); }
    
    /* ====================================================================== */
    /* 常量管理 */
    /* ====================================================================== */
    
    /**
     * @brief 添加常量
     * @param value 常量值
     * @return 常量在常量表中的索引
     */
    int AddConstant(const LuaValue& value);
    
    /**
     * @brief 查找常量索引
     * @param value 常量值
     * @return 常量索引，如果不存在返回-1
     */
    int FindConstant(const LuaValue& value) const;
    
    /**
     * @brief 获取常量
     * @param index 常量索引
     * @return 常量值的引用
     */
    const LuaValue& GetConstant(int index) const;
    
    /**
     * @brief 获取常量表
     */
    const std::vector<LuaValue>& GetConstants() const { return constants_; }
    
    /**
     * @brief 获取常量表大小
     */
    Size GetConstantCount() const { return constants_.size(); }
    
    /* ====================================================================== */
    /* 子函数管理 */
    /* ====================================================================== */
    
    /**
     * @brief 添加子函数原型
     * @param proto 子函数原型
     * @return 子函数在原型表中的索引
     */
    int AddSubProto(std::unique_ptr<Proto> proto);
    
    /**
     * @brief 获取子函数原型
     * @param index 子函数索引
     * @return 子函数原型的指针
     */
    const Proto* GetSubProto(int index) const;
    
    /**
     * @brief 获取子函数原型（可修改）
     */
    Proto* GetSubProto(int index);
    
    /**
     * @brief 获取子函数数量
     */
    Size GetSubProtoCount() const { return protos_.size(); }
    
    /**
     * @brief 获取所有子函数原型
     */
    const std::vector<std::unique_ptr<Proto>>& GetProtos() const { return protos_; }
    
    /* ====================================================================== */
    /* 上值管理 */
    /* ====================================================================== */
    
    /**
     * @brief 添加上值描述符
     * @param desc 上值描述符
     * @return 上值索引
     */
    int AddUpvalue(const UpvalueDesc& desc);
    
    /**
     * @brief 获取上值描述符
     * @param index 上值索引
     * @return 上值描述符的引用
     */
    const UpvalueDesc& GetUpvalue(int index) const;
    
    /**
     * @brief 获取上值数量
     */
    Size GetUpvalueCount() const { return upvalues_.size(); }
    
    /**
     * @brief 获取所有上值描述符
     */
    const std::vector<UpvalueDesc>& GetUpvalues() const { return upvalues_; }
    
    /* ====================================================================== */
    /* 函数属性 */
    /* ====================================================================== */
    
    /**
     * @brief 设置参数数量
     */
    void SetParameterCount(Size count) { parameter_count_ = count; }
    
    /**
     * @brief 获取参数数量
     */
    Size GetParameterCount() const { return parameter_count_; }
    
    /**
     * @brief 设置是否为可变参数函数
     */
    void SetVariadic(bool is_vararg) { is_vararg_ = is_vararg; }
    
    /**
     * @brief 检查是否为可变参数函数
     */
    bool IsVariadic() const { return is_vararg_; }
    
    /**
     * @brief 设置最大栈大小
     */
    void SetMaxStackSize(Size size) { max_stack_size_ = size; }
    
    /**
     * @brief 获取最大栈大小
     */
    Size GetMaxStackSize() const { return max_stack_size_; }
    
    /* ====================================================================== */
    /* 调试信息 */
    /* ====================================================================== */
    
    /**
     * @brief 添加局部变量信息
     */
    void AddLocalVar(const LocalVarInfo& var_info);
    
    /**
     * @brief 获取局部变量信息
     */
    const std::vector<LocalVarInfo>& GetLocalVars() const { return local_vars_; }
    
    /**
     * @brief 获取行信息
     */
    const std::vector<LineInfo>& GetLineInfo() const { return line_info_; }
    
    /**
     * @brief 获取源文件名
     */
    const std::string& GetSourceName() const { return source_name_; }
    
    /**
     * @brief 获取函数定义行号
     */
    int GetLineDefined() const { return line_defined_; }
    
    /**
     * @brief 获取函数结束行号
     */
    int GetLastLineDefined() const { return last_line_defined_; }
    
    /**
     * @brief 设置函数结束行号
     */
    void SetLastLineDefined(int line) { last_line_defined_ = line; }

private:
    // 指令序列
    std::vector<Instruction> code_;
    
    // 常量表
    std::vector<LuaValue> constants_;
    
    // 子函数原型
    std::vector<std::unique_ptr<Proto>> protos_;
    
    // 上值描述符
    std::vector<UpvalueDesc> upvalues_;
    
    // 函数属性
    Size parameter_count_;          // 参数数量
    bool is_vararg_;               // 是否可变参数
    Size max_stack_size_;          // 最大栈大小
    
    // 调试信息
    std::vector<LocalVarInfo> local_vars_;   // 局部变量信息
    std::vector<LineInfo> line_info_;        // 行号信息
    std::string source_name_;               // 源文件名
    int line_defined_;                      // 函数定义行号
    int last_line_defined_;                 // 函数结束行号
};

/* ========================================================================== */
/* 字节码生成辅助类 */
/* ========================================================================== */

/**
 * @brief 跳转指令修复器
 * 
 * 用于修复需要向前跳转的指令，如if、while等控制结构
 */
class JumpPatcher {
public:
    /**
     * @brief 构造函数
     * @param proto 目标函数原型
     */
    explicit JumpPatcher(Proto* proto) : proto_(proto) {}
    
    /**
     * @brief 记录需要修复的跳转指令
     * @param pc 指令位置
     * @return 跳转ID，用于后续修复
     */
    int RecordJump(Size pc);
    
    /**
     * @brief 修复跳转指令
     * @param jump_id 跳转ID
     * @param target_pc 目标位置
     */
    void PatchJump(int jump_id, Size target_pc);
    
    /**
     * @brief 修复跳转指令到当前位置
     * @param jump_id 跳转ID
     */
    void PatchJumpToHere(int jump_id);
    
    /**
     * @brief 获取当前代码位置
     */
    Size GetCurrentPC() const;

private:
    Proto* proto_;
    std::vector<Size> pending_jumps_;
};

/**
 * @brief 表达式上下文类型
 */
enum class ExpressionType {
    Void,           // 无值表达式
    Nil,            // nil字面量
    True,           // true字面量
    False,          // false字面量
    Constant,       // 常量
    Local,          // 局部变量
    Global,         // 全局变量
    Register,       // 寄存器中的值
    Test,           // 测试表达式（用于逻辑运算）
    Vararg          // 可变参数表达式
};

/**
 * @brief 表达式编译上下文
 * 
 * 记录表达式编译后的状态和位置信息
 */
struct ExpressionContext {
    ExpressionType type;                            // 表达式类型
    std::optional<RegisterIndex> register_index;    // 寄存器索引
    std::optional<int> constant_index;             // 常量索引
    std::vector<int> true_jumps;                   // 为真时的跳转列表
    std::vector<int> false_jumps;                  // 为假时的跳转列表
    
    /**
     * @brief 构造函数
     */
    explicit ExpressionContext(ExpressionType t = ExpressionType::Void)
        : type(t) {}
    
    /**
     * @brief 检查是否有值
     */
    bool HasValue() const {
        return type != ExpressionType::Void;
    }
    
    /**
     * @brief 检查是否是常量表达式
     */
    bool IsConstant() const {
        return type == ExpressionType::Constant ||
               type == ExpressionType::Nil ||
               type == ExpressionType::True ||
               type == ExpressionType::False;
    }
    
    /**
     * @brief 检查是否需要寄存器
     */
    bool NeedsRegister() const {
        return type == ExpressionType::Register ||
               type == ExpressionType::Local ||
               type == ExpressionType::Global;
    }
};

} // namespace lua_cpp