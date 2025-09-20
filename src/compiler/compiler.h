#pragma once

#include "bytecode.h"
#include "parser/ast.h"
#include "core/common.h"
#include "core/lua_value.h"
#include "core/error.h"
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <stack>

namespace lua_cpp {

/* ========================================================================== */
/* 前向声明 */
/* ========================================================================== */

class ASTNode;
class Expression;
class Statement;
class Program;

/* ========================================================================== */
/* 编译器错误处理 */
/* ========================================================================== */

/**
 * @brief 编译器错误类
 */
class CompilerError : public LuaError {
public:
    /**
     * @brief 构造函数
     * @param message 错误消息
     * @param line 错误行号
     * @param column 错误列号
     */
    explicit CompilerError(const std::string& message, int line = 0, int column = 0)
        : LuaError(ErrorType::Compilation, message, line, column) {}
};

/* ========================================================================== */
/* 优化配置 */
/* ========================================================================== */

/**
 * @brief 优化类型枚举
 */
enum class OptimizationType {
    ConstantFolding,        // 常量折叠
    DeadCodeElimination,    // 死代码消除
    JumpOptimization,       // 跳转优化
    LocalVariableReuse,     // 局部变量重用
    TailCallOptimization    // 尾调用优化
};

/**
 * @brief 优化配置结构
 */
struct OptimizationConfig {
    bool constant_folding = true;
    bool dead_code_elimination = true;
    bool jump_optimization = true;
    bool local_variable_reuse = true;
    bool tail_call_optimization = true;
    
    /**
     * @brief 检查是否启用指定优化
     */
    bool IsEnabled(OptimizationType type) const {
        switch (type) {
            case OptimizationType::ConstantFolding: return constant_folding;
            case OptimizationType::DeadCodeElimination: return dead_code_elimination;
            case OptimizationType::JumpOptimization: return jump_optimization;
            case OptimizationType::LocalVariableReuse: return local_variable_reuse;
            case OptimizationType::TailCallOptimization: return tail_call_optimization;
            default: return false;
        }
    }
};

/* ========================================================================== */
/* 编译上下文管理 */
/* ========================================================================== */

/**
 * @brief 局部变量信息
 */
struct LocalVariable {
    std::string name;           // 变量名
    RegisterIndex register_idx; // 寄存器索引
    int scope_level;           // 作用域层级
    bool is_captured;          // 是否被闭包捕获
    
    LocalVariable(const std::string& n, RegisterIndex reg, int level = 0)
        : name(n), register_idx(reg), scope_level(level), is_captured(false) {}
};

/**
 * @brief 上值信息
 */
struct UpvalueInfo {
    std::string name;
    int index;              // 在上值表中的索引
    bool is_local;          // 是否来自局部变量
    RegisterIndex reg_idx;   // 如果是局部变量，其寄存器索引
    
    UpvalueInfo(const std::string& n, int idx, bool local, RegisterIndex reg = 0)
        : name(n), index(idx), is_local(local), reg_idx(reg) {}
};

/**
 * @brief 作用域管理器
 */
class ScopeManager {
public:
    /**
     * @brief 进入新作用域
     */
    void EnterScope();
    
    /**
     * @brief 退出当前作用域
     * @return 退出作用域时需要释放的寄存器数量
     */
    int ExitScope();
    
    /**
     * @brief 声明局部变量
     * @param name 变量名
     * @param register_idx 寄存器索引
     */
    void DeclareLocal(const std::string& name, RegisterIndex register_idx);
    
    /**
     * @brief 查找局部变量
     * @param name 变量名
     * @return 局部变量信息，如果不存在返回nullptr
     */
    const LocalVariable* FindLocal(const std::string& name) const;
    
    /**
     * @brief 获取当前作用域层级
     */
    int GetCurrentScopeLevel() const { return scope_level_; }
    
    /**
     * @brief 获取所有局部变量
     */
    const std::vector<LocalVariable>& GetLocals() const { return locals_; }

private:
    std::vector<LocalVariable> locals_;     // 局部变量列表
    std::vector<Size> scope_markers_;       // 作用域标记
    int scope_level_ = 0;                   // 当前作用域层级
};

/**
 * @brief 寄存器分配器
 */
class RegisterAllocator {
public:
    /**
     * @brief 构造函数
     */
    RegisterAllocator() : next_register_(0), register_top_(0) {}
    
    /**
     * @brief 分配新寄存器
     * @return 分配的寄存器索引
     */
    RegisterIndex Allocate();
    
    /**
     * @brief 释放寄存器
     * @param reg 寄存器索引
     */
    void Free(RegisterIndex reg);
    
    /**
     * @brief 分配临时寄存器
     * @return 临时寄存器索引
     */
    RegisterIndex AllocateTemporary();
    
    /**
     * @brief 释放所有临时寄存器
     * @param saved_top 保存的寄存器栈顶位置
     */
    void FreeTemporaries(Size saved_top);
    
    /**
     * @brief 获取寄存器栈顶
     */
    Size GetTop() const { return register_top_; }
    
    /**
     * @brief 设置寄存器栈顶
     */
    void SetTop(Size top);
    
    /**
     * @brief 获取空闲寄存器数量
     */
    Size GetFreeCount() const;
    
    /**
     * @brief 重置分配器
     */
    void Reset();

private:
    RegisterIndex next_register_;               // 下一个分配的寄存器
    Size register_top_;                         // 寄存器栈顶
    std::vector<bool> free_registers_;          // 空闲寄存器标记
    std::stack<Size> temporary_markers_;        // 临时寄存器标记栈
};

/* ========================================================================== */
/* 编译器主类 */
/* ========================================================================== */

/**
 * @brief Lua编译器类
 * 
 * 实现Lua 5.1.5的编译器，将AST转换为字节码
 * 支持以下功能：
 * - 表达式和语句编译
 * - 寄存器分配管理
 * - 作用域和变量管理
 * - 代码优化
 * - 错误报告
 */
class Compiler {
public:
    /**
     * @brief 构造函数
     * @param config 优化配置
     * @param strict_mode 是否启用严格模式
     */
    explicit Compiler(const OptimizationConfig& config = OptimizationConfig(), 
                     bool strict_mode = false);
    
    /**
     * @brief 析构函数
     */
    ~Compiler() = default;
    
    // 禁用拷贝，允许移动
    Compiler(const Compiler&) = delete;
    Compiler& operator=(const Compiler&) = delete;
    Compiler(Compiler&&) = default;
    Compiler& operator=(Compiler&&) = default;
    
    /* ====================================================================== */
    /* 程序编译接口 */
    /* ====================================================================== */
    
    /**
     * @brief 编译完整程序
     * @param program AST根节点
     * @param source_name 源文件名
     * @return 主函数原型
     */
    std::unique_ptr<Proto> CompileProgram(const Program* program, 
                                         const std::string& source_name = "");
    
    /* ====================================================================== */
    /* 函数编译管理 */
    /* ====================================================================== */
    
    /**
     * @brief 开始编译新函数
     * @param name 函数名
     * @param parameters 参数列表
     * @param is_vararg 是否为可变参数函数
     */
    void BeginFunction(const std::string& name, 
                      const std::vector<std::string>& parameters,
                      bool is_vararg);
    
    /**
     * @brief 结束当前函数编译
     * @return 函数原型
     */
    std::unique_ptr<Proto> EndFunction();
    
    /**
     * @brief 获取当前编译的函数
     */
    Proto* GetCurrentFunction();
    
    /**
     * @brief 获取当前编译的函数（只读）
     */
    const Proto* GetCurrentFunction() const;
    
    /* ====================================================================== */
    /* 表达式编译 */
    /* ====================================================================== */
    
    /**
     * @brief 编译表达式
     * @param expr 表达式AST节点
     * @return 表达式编译上下文
     */
    ExpressionContext CompileExpression(const Expression* expr);
    
    /**
     * @brief 编译表达式到指定寄存器
     * @param expr 表达式AST节点
     * @param target_reg 目标寄存器
     * @return 表达式编译上下文
     */
    ExpressionContext CompileExpressionToRegister(const Expression* expr, 
                                                 RegisterIndex target_reg);
    
    /**
     * @brief 编译表达式作为RK值
     * @param expr 表达式AST节点
     * @return RK值编码
     */
    int CompileExpressionAsRK(const Expression* expr);
    
    /**
     * @brief 编译条件表达式
     * @param expr 表达式AST节点
     * @param true_jumps 为真时的跳转列表
     * @param false_jumps 为假时的跳转列表
     */
    void CompileCondition(const Expression* expr, 
                         std::vector<int>& true_jumps,
                         std::vector<int>& false_jumps);
    
    /* ====================================================================== */
    /* 语句编译 */
    /* ====================================================================== */
    
    /**
     * @brief 编译语句
     * @param stmt 语句AST节点
     */
    void CompileStatement(const Statement* stmt);
    
    /* ====================================================================== */
    /* 寄存器管理 */
    /* ====================================================================== */
    
    /**
     * @brief 分配寄存器
     * @return 分配的寄存器索引
     */
    RegisterIndex AllocateRegister();
    
    /**
     * @brief 释放寄存器
     * @param reg 寄存器索引
     */
    void FreeRegister(RegisterIndex reg);
    
    /**
     * @brief 分配临时寄存器
     * @return 临时寄存器索引
     */
    RegisterIndex AllocateTemporary();
    
    /**
     * @brief 释放临时寄存器
     * @param saved_top 保存的寄存器栈顶
     */
    void FreeTemporaries(Size saved_top);
    
    /**
     * @brief 获取寄存器栈顶
     */
    Size GetRegisterTop() const;
    
    /**
     * @brief 设置寄存器栈顶
     */
    void SetRegisterTop(Size top);
    
    /**
     * @brief 获取空闲寄存器数量
     */
    Size GetFreeRegisterCount() const;
    
    /* ====================================================================== */
    /* 变量管理 */
    /* ====================================================================== */
    
    /**
     * @brief 声明局部变量
     * @param name 变量名
     * @return 分配的寄存器索引
     */
    RegisterIndex DeclareLocalVariable(const std::string& name);
    
    /**
     * @brief 查找局部变量
     * @param name 变量名
     * @return 局部变量信息，如果不存在返回nullptr
     */
    const LocalVariable* FindLocalVariable(const std::string& name) const;
    
    /**
     * @brief 查找上值
     * @param name 变量名
     * @return 上值信息，如果不存在返回nullptr
     */
    const UpvalueInfo* FindUpvalue(const std::string& name) const;
    
    /**
     * @brief 添加上值
     * @param name 变量名
     * @param is_local 是否来自局部变量
     * @param index 索引
     * @return 上值在表中的索引
     */
    int AddUpvalue(const std::string& name, bool is_local, int index);
    
    /* ====================================================================== */
    /* 作用域管理 */
    /* ====================================================================== */
    
    /**
     * @brief 进入新作用域
     */
    void EnterScope();
    
    /**
     * @brief 退出当前作用域
     */
    void ExitScope();
    
    /* ====================================================================== */
    /* 优化控制 */
    /* ====================================================================== */
    
    /**
     * @brief 检查是否启用指定优化
     * @param type 优化类型
     * @return 是否启用
     */
    bool IsOptimizationEnabled(OptimizationType type) const;
    
    /**
     * @brief 设置优化选项
     * @param type 优化类型
     * @param enabled 是否启用
     */
    void SetOptimization(OptimizationType type, bool enabled);
    
    /**
     * @brief 检查是否为严格模式
     */
    bool IsStrictMode() const { return strict_mode_; }
    
    /**
     * @brief 设置严格模式
     */
    void SetStrictMode(bool strict) { strict_mode_ = strict; }
    
    /* ====================================================================== */
    /* 跳转管理 */
    /* ====================================================================== */
    
    /**
     * @brief 生成跳转指令
     * @return 跳转指令的位置
     */
    int EmitJump();
    
    /**
     * @brief 修复跳转指令
     * @param jump_pc 跳转指令位置
     * @param target_pc 目标位置
     */
    void PatchJump(int jump_pc, Size target_pc);
    
    /**
     * @brief 修复跳转指令到当前位置
     * @param jump_pc 跳转指令位置
     */
    void PatchJumpToHere(int jump_pc);
    
    /**
     * @brief 修复跳转列表
     * @param jumps 跳转列表
     * @param target_pc 目标位置
     */
    void PatchJumpList(const std::vector<int>& jumps, Size target_pc);
    
    /**
     * @brief 修复跳转列表到当前位置
     * @param jumps 跳转列表
     */
    void PatchJumpListToHere(const std::vector<int>& jumps);
    
    /**
     * @brief 连接跳转列表
     * @param list1 第一个跳转列表
     * @param list2 第二个跳转列表
     * @return 合并后的跳转列表
     */
    std::vector<int> ConcatenateJumpLists(const std::vector<int>& list1,
                                         const std::vector<int>& list2);

private:
    /* ====================================================================== */
    /* 内部编译方法 */
    /* ====================================================================== */
    
    // 表达式编译方法
    ExpressionContext CompileLiteralExpression(const Expression* expr);
    ExpressionContext CompileBinaryExpression(const Expression* expr);
    ExpressionContext CompileUnaryExpression(const Expression* expr);
    ExpressionContext CompileVariableExpression(const Expression* expr);
    ExpressionContext CompileCallExpression(const Expression* expr);
    ExpressionContext CompileTableExpression(const Expression* expr);
    ExpressionContext CompileFunctionExpression(const Expression* expr);
    
    // 语句编译方法
    void CompileExpressionStatement(const Statement* stmt);
    void CompileBlockStatement(const Statement* stmt);
    void CompileAssignmentStatement(const Statement* stmt);
    void CompileLocalDeclaration(const Statement* stmt);
    void CompileIfStatement(const Statement* stmt);
    void CompileWhileStatement(const Statement* stmt);
    void CompileForStatement(const Statement* stmt);
    void CompileReturnStatement(const Statement* stmt);
    void CompileBreakStatement(const Statement* stmt);
    
    // 优化方法
    ExpressionContext TryConstantFolding(const Expression* expr);
    void OptimizeDeadCode();
    void OptimizeJumps();
    
    // 辅助方法
    void EmitInstruction(OpCode op, int a = 0, int b = 0, int c = 0, int line = 0);
    void EmitABx(OpCode op, int a, int bx, int line = 0);
    void EmitAsBx(OpCode op, int a, int sbx, int line = 0);
    
    Size GetCurrentPC() const;
    void SetInstructionArgument(Size pc, char arg, int value);
    
    // 错误报告
    void ReportError(const std::string& message, int line = 0, int column = 0);
    
    /* ====================================================================== */
    /* 成员变量 */
    /* ====================================================================== */
    
    // 函数编译栈
    std::stack<std::unique_ptr<Proto>> function_stack_;
    
    // 寄存器分配器
    RegisterAllocator register_allocator_;
    
    // 作用域管理器
    ScopeManager scope_manager_;
    
    // 上值信息
    std::vector<UpvalueInfo> upvalues_;
    
    // 优化配置
    OptimizationConfig optimization_config_;
    
    // 编译选项
    bool strict_mode_;                  // 严格模式
    std::string current_source_name_;   // 当前源文件名
    int current_line_;                  // 当前行号
    
    // 控制流管理
    std::stack<std::vector<int>> break_jumps_;      // break跳转栈
    std::stack<std::vector<int>> continue_jumps_;   // continue跳转栈
};

/* ========================================================================== */
/* 编译器工厂函数 */
/* ========================================================================== */

/**
 * @brief 创建标准编译器
 * @param enable_optimizations 是否启用优化
 * @param strict_mode 是否启用严格模式
 * @return 编译器实例
 */
std::unique_ptr<Compiler> CreateStandardCompiler(bool enable_optimizations = true,
                                                 bool strict_mode = false);

/**
 * @brief 创建调试编译器
 * @return 编译器实例（关闭优化，启用调试信息）
 */
std::unique_ptr<Compiler> CreateDebugCompiler();

/**
 * @brief 编译Lua程序
 * @param program AST根节点
 * @param source_name 源文件名
 * @param config 优化配置
 * @return 主函数原型
 */
std::unique_ptr<Proto> CompileProgram(const Program* program,
                                     const std::string& source_name = "",
                                     const OptimizationConfig& config = OptimizationConfig());

} // namespace lua_cpp